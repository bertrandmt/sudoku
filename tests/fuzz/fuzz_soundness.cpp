// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
//
// Randomised soundness fuzz for the analyzer.
//
// Why this exists, and why the other two suites do not cover it. A newly added
// technique fires on only the one or two boards added alongside it: Finned X-Wing
// arrived with `P_fx` plus its own whitebox cases, so both suites went green
// having exercised the new rule on a single real position. Green proved the rule
// works *there*. It could not say the rule is *sound* -- that it never removes a
// candidate the solution needs -- because soundness is a claim over positions,
// and a fixture is one position.
//
// So: generate puzzles, keep the ones with a unique solution, and step the real
// solver through each of them checking the one invariant that must never break --
// after every step, every unsolved cell still offers its solution value, and
// every solved cell holds it. A wrong elimination shows up as a cell whose
// candidate set has lost the digit the solution needs, at the exact step that
// dropped it, with the technique that fired named.
//
// Deterministic by default: the seed is fixed, printed on every run, and
// overridable, so a failure is a reproduction recipe rather than a story about a
// board nobody can find again. Nothing here is a benchmark; timing is not
// measured and the generator is not tuned for speed.
//
// Framework-free, same as tests/unit: this repository carries no test dependency
// and no scripting runtime, and a fuzz harness is not a good enough reason to
// introduce either. It links the library objects and drives the public Solver.
//
// Usage: fuzz_soundness [puzzles] [seed]

#include "solver.h"
#include "board.h"
#include "cell.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// The analyzer reads this application-global to decide whether to narrate; it is
// normally defined by the REPL's main, which this binary does not link. Kept
// false: the per-step narration would bury the report, and the checks below read
// the board rather than the narration.
bool sVerbose = false;

namespace {

using Grid = std::array<int, 81>;   // 0 = empty, 1-9 = value

constexpr size_t kDefaultPuzzles = 200;
constexpr uint64_t kDefaultSeed = 20260805;

bool legal_at(const Grid &g, size_t idx, int v) {
    const size_t row = idx / 9, col = idx % 9;
    for (size_t i = 0; i < 9; i++) {
        if (g[row * 9 + i] == v) return false;
        if (g[i * 9 + col] == v) return false;
    }
    const size_t br = (row / 3) * 3, bc = (col / 3) * 3;
    for (size_t r = br; r < br + 3; r++)
        for (size_t c = bc; c < bc + 3; c++)
            if (g[r * 9 + c] == v) return false;
    return true;
}

// Count solutions, abandoning the search once `limit` are found. Uniqueness only
// ever needs limit=2, and stopping there keeps the dig loop below affordable.
size_t count_solutions(Grid &g, size_t limit) {
    size_t idx = 82;
    for (size_t i = 0; i < 81; i++)
        if (g[i] == 0) { idx = i; break; }
    if (idx == 82) return 1;

    size_t found = 0;
    for (int v = 1; v <= 9 && found < limit; v++) {
        if (!legal_at(g, idx, v)) continue;
        g[idx] = v;
        found += count_solutions(g, limit - found);
        g[idx] = 0;
    }
    return found;
}

// A complete, valid grid, filled by backtracking over a shuffled value order so
// successive calls differ.
bool fill(Grid &g, size_t idx, std::mt19937_64 &rng) {
    if (idx == 81) return true;
    std::array<int, 9> vals{1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(vals.begin(), vals.end(), rng);
    for (int v : vals) {
        if (!legal_at(g, idx, v)) continue;
        g[idx] = v;
        if (fill(g, idx + 1, rng)) return true;
        g[idx] = 0;
    }
    return false;
}

// Remove clues from `solution` in random order, keeping each removal only while
// the puzzle still has exactly one completion. The result therefore has
// `solution` as its unique solution -- which is what makes it a usable oracle.
Grid dig(const Grid &solution, std::mt19937_64 &rng) {
    Grid puzzle = solution;
    std::array<size_t, 81> order;
    for (size_t i = 0; i < 81; i++) order[i] = i;
    std::shuffle(order.begin(), order.end(), rng);
    for (size_t idx : order) {
        const int saved = puzzle[idx];
        puzzle[idx] = 0;
        Grid probe = puzzle;
        if (count_solutions(probe, 2) != 1) puzzle[idx] = saved;
    }
    return puzzle;
}

std::string to_desc(const Grid &g) {          // Board's form-2 description
    std::string s = ".";
    for (int v : g) s += (v == 0 ? '.' : static_cast<char>('0' + v));
    return s;
}

// Parse Board::print_candidates output: one '~'-led line per row, each token
// either a solved digit or the concatenated candidate digits ('-' if none).
std::vector<std::vector<int>> parse_candidates(const std::string &dump) {
    std::vector<std::vector<int>> cells;
    std::istringstream lines(dump);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.empty() || line[0] != '~') continue;
        std::istringstream toks(line.substr(1));
        std::string tok;
        while (toks >> tok) {
            std::vector<int> cand;
            for (char ch : tok)
                if (ch >= '1' && ch <= '9') cand.push_back(ch - '0');
            cells.push_back(cand);   // '-' yields an empty set, itself a failure
        }
    }
    return cells;
}

// Every technique tag whose action lines appear in `narration`, with counts. The
// solver prints one "[XX] [r, c] ..." line per action it takes, so this reports
// which rules a puzzle actually exercised without reaching into the Analyzer.
std::map<std::string, size_t> tags_in(const std::string &narration) {
    std::map<std::string, size_t> counts;
    std::istringstream lines(narration);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.size() < 6 || line[0] != '[' || line[3] != ']' || line[4] != ' ') continue;
        if (line[5] != '[') continue;               // an action line, not a dump line
        counts[line.substr(1, 2)]++;
    }
    return counts;
}

// Every technique tag the binary reports, in cascade order. Analyzer::registry()
// is private and this harness has no friend hook, so read the tags out of the
// analyzer dump instead -- one "[XX](n) {...}" line per registry entry. Same
// approach tests/run.sh takes for the same reason: ask the binary rather than
// keep a list that can disagree with it.
std::vector<std::string> cascade_tags() {
    Solver probe(std::string(1, '.') + std::string(81, '.'));
    std::ostringstream dump;
    dump << probe;
    std::vector<std::string> tags;
    std::istringstream lines(dump.str());
    std::string line;
    while (std::getline(lines, line))
        if (line.size() > 4 && line[0] == '[' && line[3] == ']' && line[4] == '(')
            tags.push_back(line.substr(1, 2));
    return tags;
}

struct Report {
    size_t puzzles = 0, solved = 0, steps = 0, violations = 0;
    std::map<std::string, size_t> actions;    // tag -> action lines
    std::map<std::string, size_t> puzzles_with;  // tag -> puzzles exercising it
};

// Step one puzzle to a standstill, checking the invariant after every step.
// Returns false on the first violation, having reported it.
bool run_one(const Grid &puzzle, const Grid &solution, uint64_t seed, Report &rep) {
    Solver solver(to_desc(puzzle));

    // The solver narrates its actions to stdout unconditionally (only the
    // *detail* is gated on sVerbose), so capture it: 200 puzzles of narration
    // would bury the report, and the capture doubles as the record of which
    // techniques fired.
    std::ostringstream narration;
    std::streambuf *saved = std::cout.rdbuf(narration.rdbuf());

    bool ok = true;
    size_t step = 0;
    while (solver.solve_one_step(false)) {
        step++;
        std::ostringstream dump;
        solver.print_candidates(dump);
        const auto cells = parse_candidates(dump.str());

        if (cells.size() != 81) {
            std::cout.rdbuf(saved);
            std::cerr << "  malformed candidate dump: " << cells.size() << " cells\n";
            return false;
        }
        for (size_t i = 0; i < 81 && ok; i++) {
            const int want = solution[i];
            const auto &cand = cells[i];
            if (std::find(cand.begin(), cand.end(), want) == cand.end()) {
                std::cout.rdbuf(saved);
                std::cerr << "SOUNDNESS VIOLATION at step " << step << ", cell R"
                          << (i / 9 + 1) << "C" << (i % 9 + 1) << ": solution needs " << want
                          << " but the cell offers {";
                for (size_t k = 0; k < cand.size(); k++) std::cerr << (k ? "," : "") << cand[k];
                std::cerr << "}\n  seed:   " << seed
                          << "\n  puzzle: " << to_desc(puzzle).substr(1)
                          << "\n  solved: " << to_desc(solution).substr(1) << "\n";
                std::cerr << "  narration of the step that did it:\n";
                // The last step's lines are what matters; print the tail.
                std::istringstream lines(narration.str());
                std::vector<std::string> all;
                for (std::string l; std::getline(lines, l); ) all.push_back(l);
                for (size_t k = all.size() > 12 ? all.size() - 12 : 0; k < all.size(); k++)
                    std::cerr << "    " << all[k] << "\n";
                ok = false;
            }
        }
        if (!ok) break;
    }
    if (ok) std::cout.rdbuf(saved);

    rep.steps += step;
    if (ok && solver.solved()) rep.solved++;
    for (const auto &[tag, n] : tags_in(narration.str())) {
        rep.actions[tag] += n;
        rep.puzzles_with[tag]++;
    }
    return ok;
}

} // namespace

int main(int argc, char **argv) {
    const size_t puzzles = (argc > 1) ? std::strtoul(argv[1], nullptr, 10) : kDefaultPuzzles;
    const uint64_t seed  = (argc > 2) ? std::strtoull(argv[2], nullptr, 10) : kDefaultSeed;

    std::cout << "soundness fuzz: " << puzzles << " puzzles, seed " << seed
              << " (reproduce with: fuzz_soundness " << puzzles << " " << seed << ")\n";

    std::mt19937_64 rng(seed);
    Report rep;

    for (size_t i = 0; i < puzzles; i++) {
        Grid solution{};
        if (!fill(solution, 0, rng)) { std::cerr << "generator failed to fill a grid\n"; return 2; }
        const Grid puzzle = dig(solution, rng);
        rep.puzzles++;
        if (!run_one(puzzle, solution, seed, rep)) {
            rep.violations++;
            break;      // the first violation is the interesting one; stop for a clean report
        }
    }

    std::cout << "\npuzzles: " << rep.puzzles
              << "   fully solved: " << rep.solved
              << "   steps: " << rep.steps << "\n";
    std::cout << "techniques exercised (actions / puzzles):\n";
    if (rep.actions.empty()) std::cout << "  none -- the solver took no action at all\n";
    for (const auto &[tag, n] : rep.actions)
        std::cout << "  [" << tag << "]  " << n << " / " << rep.puzzles_with[tag] << "\n";

    // A technique the fuzz never reached is not evidence about that technique. Say
    // so rather than letting a green run imply coverage it does not have.
    std::vector<std::string> untouched;
    for (const auto &tag : cascade_tags())
        if (!rep.actions.count(tag)) untouched.push_back(tag);
    if (!untouched.empty()) {
        std::cout << "NOT exercised by this run (no evidence either way):";
        for (const auto &t : untouched) std::cout << " [" << t << "]";
        std::cout << "\n  raise the puzzle count to reach the rarer rules\n";
    }

    if (rep.violations) {
        std::cout << "\nFAIL: " << rep.violations << " soundness violation(s)\n";
        return 1;
    }
    std::cout << "\nok: no step dropped a true candidate or placed a wrong value\n";
    return 0;
}
