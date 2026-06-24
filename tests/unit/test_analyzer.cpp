// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
//
// Whitebox unit tests for the Analyzer.
//
// The black-box suite (tests/run.sh) drives the compiled REPL and can only
// reach an analyzer technique when a real solve happens to route through it.
// Some code paths are hard to provoke that way -- notably the column-based
// Swordfish branch, which fires only when no row-based Swordfish is anchored
// earlier in cell-iteration order. These tests construct a candidate grid
// directly and call the (private) Analyzer entry points via the AnalyzerTest
// friend, so a single technique can be exercised on a position designed for it.
//
// Framework-free on purpose: this matches the project's no-dependency testing
// style. Each CHECK records a line; a nonzero exit code means a failure.

#include "board.h"
#include "analyzer.h"
#include "cell.h"
#include "coord.h"

#include <iostream>
#include <string>
#include <vector>

// The analyzer reads this application-global to decide whether to narrate its
// steps; it is normally defined in the REPL main (sudoku-solver.cpp), which this
// test binary does not link, so we supply it here. The tests keep it false.
bool sVerbose = false;

// Friend hook: the only thing allowed to read Analyzer internals.
struct AnalyzerTest {
    static bool find_swordfish(Analyzer &a, const Cell &c, const Value &v) { return a.find_swordfish(c, v); }
    static bool act_on_swordfish(Analyzer &a)                              { return a.act_on_swordfish(); }
    static size_t swordfish_count(const Analyzer &a)                       { return a.mSwordfish.size(); }
    static bool   swordfish_row_based(const Analyzer &a)                   { return a.mSwordfish.at(0).is_row_based; }
    static Value  swordfish_value(const Analyzer &a)                       { return a.mSwordfish.at(0).value; }
};

namespace {

int failures = 0;
void check(bool cond, const std::string &msg) {
    if (cond) { std::cout << "  ok    " << msg << "\n"; }
    else      { std::cout << "  FAIL  " << msg << "\n"; ++failures; }
}

// An empty 9x9 board: every cell a note carrying all nine candidates.
Board empty_board() { return Board(std::string(82, '.')); }

// Reduce a board so that `value` is a candidate ONLY in the listed (row,col)
// cells (0-based); it is struck everywhere else. Other candidates are left
// untouched -- the Swordfish logic only inspects `value`.
void confine_value(Board &board, Value value, const std::vector<std::pair<size_t,size_t>> &keep) {
    for (size_t r = 0; r < Board::height; ++r) {
        for (size_t c = 0; c < Board::width; ++c) {
            bool keep_it = false;
            for (auto const &k : keep) if (k.first == r && k.second == c) { keep_it = true; break; }
            if (!keep_it) board.clear_note_at(r, c, value);
        }
    }
}

bool has_candidate(const Board &board, size_t r, size_t c, Value v) {
    return board.cells()[r * Board::width + c].check(v);
}

// A column-based Swordfish on value 8.
//
// Candidate 8 lives in base columns {0,1,2}, confined to rows {0,1,2}:
//
//        c0 c1 c2 c3 c4 c5
//   r0    8  .  8  .  8  .      <- (0,4) is outside the base columns
//   r1    8  8  .  .  .  8      <- (1,5) is outside the base columns
//   r2    .  8  8  .  .  .
//
// Each base column has two 8s; their rows union to exactly {0,1,2}. So in those
// three rows, 8 must occupy the three base columns, and the stray 8s at (0,4)
// and (1,5) are eliminable. Note every cover row holds exactly three 8s (two in
// the pattern, one outside): the earlier ">3 candidates in the line" test would
// have missed this entirely, so this doubles as the regression lock for that fix.
void test_swordfish_column_based() {
    std::cout << "[swordfish] column-based detection, action, and the tight-cover-line fix\n";
    Board board = empty_board();
    const Value V = kEight;
    confine_value(board, V, {
        {0,0},{1,0},        // column 0: rows {0,1}
        {1,1},{2,1},        // column 1: rows {1,2}
        {0,2},{2,2},        // column 2: rows {0,2}
        {0,4},{1,5},        // strays to be eliminated
    });

    Analyzer analyzer(board);
    // Anchor at (0,0): find tries row-based first (no row-based pattern exists
    // here, the strays break the column-union) then column-based, which fires.
    bool found = AnalyzerTest::find_swordfish(analyzer, board.cells()[0], V);
    check(found, "column Swordfish detected on a position with only tight cover lines");
    check(AnalyzerTest::swordfish_count(analyzer) == 1, "exactly one Swordfish recorded");
    if (AnalyzerTest::swordfish_count(analyzer) == 1) {
        check(!AnalyzerTest::swordfish_row_based(analyzer), "recorded Swordfish is column-based");
        check(AnalyzerTest::swordfish_value(analyzer) == V, "recorded Swordfish is for value 8");
    }

    bool acted = AnalyzerTest::act_on_swordfish(analyzer);
    check(acted, "act_on_swordfish reports an elimination");
    check(!has_candidate(board, 0, 4, V), "stray 8 at (0,4) eliminated");
    check(!has_candidate(board, 1, 5, V), "stray 8 at (1,5) eliminated");
    // The six pattern cells must be untouched.
    check(has_candidate(board, 0, 0, V) && has_candidate(board, 1, 0, V)
       && has_candidate(board, 1, 1, V) && has_candidate(board, 2, 1, V)
       && has_candidate(board, 0, 2, V) && has_candidate(board, 2, 2, V),
          "all six pattern cells kept candidate 8");
}

// The same base columns, but with the two stray 8s removed: a Swordfish shape
// with nothing to eliminate. Detection must decline -- recording it would lead
// act_on_swordfish to assert (it requires a real elimination).
void test_swordfish_no_elimination() {
    std::cout << "[swordfish] a pattern with nothing to eliminate is not recorded\n";
    Board board = empty_board();
    const Value V = kEight;
    confine_value(board, V, {
        {0,0},{1,0},
        {1,1},{2,1},
        {0,2},{2,2},
    });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_swordfish(analyzer, board.cells()[0], V);
    check(!found, "no Swordfish reported when there is nothing to eliminate");
    check(AnalyzerTest::swordfish_count(analyzer) == 0, "no Swordfish recorded");
}

} // namespace

int main() {
    test_swordfish_column_based();
    test_swordfish_no_elimination();

    std::cout << "----------------------------------------\n";
    if (failures == 0) { std::cout << "unit: all checks passed\n"; return 0; }
    std::cout << "unit: " << failures << " check(s) failed\n";
    return 1;
}
