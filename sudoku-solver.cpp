// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "cell.h"
#include "solverstate.h"
#include "solver.h"
#include "verbose.h"

#include <iostream>
#include <string>

bool sVerbose = false;

namespace {

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
              << "  '<' or ','    go back one auto-solveing step" << std::endl
              << "  '!'           reset the solver to its initial state" << std::endl
              << "  'r'           run auto-solving until blocked (or done)" << std::endl
              << "  's'           run auto-solving using only 'naked' and 'singles' heuristics" << std::endl
              << "  'xrcv'        edit note at row 'r' and column 'c' and unset value 'v'" << std::endl
              << std::endl
              << "Other commands:" << std::endl
              << "  'p'           print the board in a compact format" << std::endl
              << "  'v'           toggle verbosity for board analysis" << std::endl
              << std::endl;
}

bool routine(Solver::ptr &solver) {
    bool done = false;

    std::string line;
    std::cout << "λ " << std::flush;
    std::getline(std::cin, line);
    if (!std::cin) {
        std::cout << std::endl;
        done = true;
        return done;
    }

    // no whitespace we care to make use of
    std::string nowsline(line, 0);
    nowsline.erase(
            std::remove_if(nowsline.begin(), nowsline.end(), [](auto c){ return std::isspace(c); }),
            nowsline.end());
    if (nowsline.size() == 0) { return done; }

    switch (nowsline[0]) {
        case '#': // it's a comment
            std::cout << line << std::endl;
            return done;

        case 'n': { // it's a new game
            Solver::ptr s(new Solver(nowsline.substr(1)));
            if (!s->isValid()) {
                help();
                break;
            }
            solver = s;
            std::cout << *solver << std::endl;
            }
            break;

        case '.':
        case '>': // auto-solve one step
            if (!solver->isValid()) { help(); break; }
            if (solver->solve_one_step(false)) { std::cout << *solver << std::endl; }
            else                               { std::cout << "???" << std::endl; }
            break;

        case ',':
        case '<': // back one step
            if (!solver->isValid()) { help(); break; }
            if (solver->back_one_step()) { std::cout << *solver << std::endl; }
            break;

        case 'r':
        case 'R': // auto-solve until blocked (or finished)
            if (!solver->isValid()) { help(); break; }
            if (solver->solve()) { std::cout << *solver << std::endl; }
            break;

        case 's':
        case 'S': // auto-solve with single heuristics only
            if (!solver->isValid()) { help(); break; }
            if (solver->solve_singles()) { std::cout << *solver << std::endl; }
            break;

        case '!': // reset
            if (!solver->isValid()) { help(); break; }
            if (solver->reset()) { std::cout << *solver << std::endl; }
            break;

        case 'x':
        case 'X': // edit a note
            if (!solver->isValid()) { help(); break; }
            if (solver->edit_note(nowsline.substr(1))) { std::cout << *solver << std::endl; }
            else                                       { help(); }
            break;

        case '=': // set a value
            if (!solver->isValid()) { help(); break; }
            if (solver->set_value(nowsline.substr(1))) { std::cout << *solver << std::endl; }
            else                                       { help(); }
            break;

        case 'p':
        case 'P': // print the board in . notation
            if (!solver->isValid()) { help(); break; }
            solver->print_current_state(std::cout);
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
    Solver::ptr solver(new Solver());

    bool done = false;

    do {
        done = routine(solver);
    } while (!done);

    return 0;
}
