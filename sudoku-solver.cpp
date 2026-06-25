// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "cell.h"
#include "completion.h"
#include "solverstate.h"
#include "solver.h"
#include "verbose.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <editline/readline.h>

bool sVerbose = false;

namespace {

bool sInteractive = true;

// Path to the persistent command-history file, computed once at startup.
std::string history_path() {
    const char *home = std::getenv("HOME");
    if (!home) return std::string();
    return std::string(home) + "/.sudoku-solver_history";
}

// Read one line of input. On an interactive terminal this uses libedit's
// readline() for in-line editing and up/down history recall; piped input
// falls back to std::getline. Returns false on end-of-input (the EOF
// newline is emitted only on the interactive path, where it tidies the
// prompt; scripted runs need no trailing blank line).
bool read_line(std::string &line) {
    if (!sInteractive) {
        std::getline(std::cin, line);
        return static_cast<bool>(std::cin);
    }

    char *raw = readline("λ ");
    if (!raw) {            // EOF (Ctrl-D)
        std::cout << std::endl;
        return false;
    }
    line.assign(raw);

    // Record non-empty lines in history, skipping consecutive duplicates.
    static std::string last;
    if (!line.empty() && line != last) {
        add_history(raw);
        last = line;
    }

    std::free(raw);
    return true;
}

// ---------------------------------------------------------------------------
// Interactive tab completion for the '=' and 'x' edit commands.
//
// libedit's completion callbacks are C function pointers with no user-data
// parameter, so the active solver is published here as a non-owning pointer and
// refreshed before each readline() call (see read_line's caller). The pure
// match logic lives in complete_move() (completion.h); everything below is glue:
// pull the verb and typed digits out of the line buffer, ask complete_move()
// for the legal continuations, and hand them back to libedit. The piped
// (getline) path never installs any of this.
// ---------------------------------------------------------------------------

// Non-owning view of the active solver. Null until the first board is loaded.
const Solver *sCompletionSolver = nullptr;

// If the current line (up to the cursor) is a '=' or 'x' edit command, split it
// into the verb and the argument digits typed so far and return true. Returns
// false for any other line, so completion stays inert for the single-character
// verbs (which have nothing worth completing).
bool parse_edit_line(std::string &verb, std::string &partial) {
    if (!rl_line_buffer) return false;
    std::string line(rl_line_buffer, rl_point);   // only what precedes the cursor
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) return false;
    char v = line[start];
    if (v != '=' && v != 'x' && v != 'X') return false;
    verb.assign(1, v);
    partial = line.substr(start + 1);
    return true;
}

// libedit completion generator: called repeatedly for one word, with state==0
// on the first call. We compute the whole match list once and dole it out one
// strdup'd entry per call, NULL when exhausted. The candidates are full words
// (verb + digits) so they match the word libedit extracted from the buffer.
char *move_completion_generator(const char * /*text*/, int state) {
    static std::vector<std::string> matches;
    static size_t idx;
    if (state == 0) {
        matches.clear();
        idx = 0;
        std::string verb, partial;
        if (sCompletionSolver && parse_edit_line(verb, partial)) {
            for (const auto &m : complete_move(*sCompletionSolver, partial)) {
                matches.push_back(verb + m);
            }
        }
    }
    if (idx >= matches.size()) return nullptr;
    return strdup(matches[idx++].c_str());
}

// Entry point installed as rl_attempted_completion_function. Suppresses the
// default filename fallback and the trailing space (the argument is one
// concatenated token, e.g. "=137"), then defers to the generator.
char **move_completion(const char *text, int /*start*/, int /*end*/) {
    rl_attempted_completion_over = 1;     // no filename fallback
    rl_completion_append_character = '\0'; // no trailing space after a match
    std::string verb, partial;
    if (!sCompletionSolver || !parse_edit_line(verb, partial)) return nullptr;
    return rl_completion_matches(text, move_completion_generator);
}

void help(void) {
    std::cout << "New game commands:" << std::endl
              << "  'n'          Start a new game" << std::endl
              << "               Enter table entries in one of two formats:" << std::endl
              << "                 ;rcv[;...]" << std::endl
              << "                   where:" << std::endl
              << "                     * \"r\" is the row, with value between 1 and 9, and" << std::endl
              << "                     * \"c\" is the column, with value between 1 and 9, and" << std::endl
              << "                     * \"v\" is the value, with value between 1 and 9." << std::endl
              << "                   Enter all entries, on one line, separated by semicolons." << std::endl
              << "                 .[v|.]*" << std::endl
              << "                   where:" << std::endl
              << "                     * \"v\" is a value between 1 and 9, and" << std::endl
              << "                     * \".\" indicates an unset cell." << std::endl
              << "                   All 81 cells in a board must be entered." << std::endl
              << std::endl
              << "Solver commands:" << std::endl
              << "  '>' or '.'    run one step of auto-solving" << std::endl
              << "  '<' or ','    go back one auto-solving step" << std::endl
              << "  '!'           reset the solver to its initial state" << std::endl
              << "  'r'           run auto-solving until blocked (or done)" << std::endl
              << "  's'           run auto-solving using only 'naked' and 'singles' heuristics" << std::endl
              << "  'xrcv'        edit note at row 'r' and column 'c' and unset value 'v'" << std::endl
              << "  '=rcv'        set cell at row 'r' and column 'c' to value 'v'" << std::endl
              << "                (press Tab after 'x'/'=' to complete r, c and v from the board)" << std::endl
              << std::endl
              << "Other commands:" << std::endl
              << "  'p'           print the board in a compact format" << std::endl
              << "  'c'           print per-cell candidates (machine-readable)" << std::endl
              << "  'v'           toggle verbosity for board analysis" << std::endl
              << std::endl;
}

bool routine(Solver::ptr &solver) {
    bool done = false;

    // Refresh the completer's view of the board before each read. Non-owning;
    // null until the first board is loaded. Harmless on the piped path, where no
    // completion callback is installed.
    sCompletionSolver = solver.get();

    std::string line;
    if (!read_line(line)) {
        done = true;
        return done;
    }

    // no whitespace we care to make use of
    std::string nowsline(line, 0);
    nowsline.erase(
            std::remove_if(nowsline.begin(), nowsline.end(), [](unsigned char c){ return std::isspace(c); }),
            nowsline.end());
    if (nowsline.size() == 0) { return done; }

    switch (nowsline[0]) {
        case '#': // it's a comment
            std::cout << line << std::endl;
            return done;

        case 'n': { // it's a new game
            try {
                solver.reset(new Solver(nowsline.substr(1)));
            }
            catch (const std::runtime_error &e) {
                std::cout << e.what() << std::endl;
                help();
                break;
            }
            assert(solver);
            std::cout << *solver << std::endl;
            }
            break;

        case '.':
        case '>': // auto-solve one step
            if (!solver) { help(); break; }
            if (solver->solved()) break;
            if (solver->solve_one_step(false)) { std::cout << *solver << std::endl; }
            else                               { std::cout << "???" << std::endl; }
            break;

        case ',':
        case '<': // back one step
            if (!solver) { help(); break; }
            if (solver->back_one_step()) { std::cout << *solver << std::endl; }
            break;

        case 'r':
        case 'R': // auto-solve until blocked (or finished)
            if (!solver) { help(); break; }
            if (solver->solve()) { std::cout << *solver << std::endl; }
            break;

        case 's':
        case 'S': // auto-solve with single heuristics only
            if (!solver) { help(); break; }
            if (solver->solve_singles()) { std::cout << *solver << std::endl; }
            break;

        case '!': // reset
            if (!solver) { help(); break; }
            if (solver->reset()) { std::cout << *solver << std::endl; }
            break;

        case 'x':
        case 'X': // edit a note
            if (!solver) { help(); break; }
            if (solver->edit_note(nowsline.substr(1))) { std::cout << *solver << std::endl; }
            else                                       { help(); }
            break;

        case '=': // set a value
            if (!solver) { help(); break; }
            if (solver->set_value(nowsline.substr(1))) { std::cout << *solver << std::endl; }
            else                                       { help(); }
            break;

        case 'p':
        case 'P': // print the board in . notation
            if (!solver) { help(); break; }
            solver->print_current_state(std::cout);
            break;

        case 'c':
        case 'C': // print per-cell candidates in a machine-readable form
            if (!solver) { help(); break; }
            solver->print_candidates(std::cout);
            break;

        case 'v':
        case 'V': // toggle verbosity of analysis steps
            sVerbose = !sVerbose;
            std::cout << "Verbose analysis: " << (sVerbose ? "ON" : "OFF") << std::endl;
            break;

        default:
            help();
            break;
    }

    return done;
}

} // namespace anonymous

int main(void) {
    Solver::ptr solver;

    bool done = false;

    // Honor the environment's locale so libedit measures the multibyte
    // prompt ("λ ") at its true display width and keeps the cursor aligned.
    std::setlocale(LC_CTYPE, "");

    // interactive?
    sInteractive = isatty(fileno(stdin));

    // Load persistent command history (interactive sessions only).
    std::string histfile;
    if (sInteractive) {
        using_history();
        stifle_history(1000);
        histfile = history_path();
        if (!histfile.empty()) read_history(histfile.c_str());

        // Context-aware Tab completion for the '=' and 'x' edit arguments.
        rl_attempted_completion_function = move_completion;
        // Keep the whole edit command as a single completion word. '=' is in
        // libedit's default word-break set, which would otherwise split "=13"
        // into "=" and "13" and strand the verb from its digits.
        rl_basic_word_break_characters = const_cast<char *>(" \t\n");
    }

    do {
        done = routine(solver);
    } while (!done);

    if (!histfile.empty()) write_history(histfile.c_str());

    return 0;
}
