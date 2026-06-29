// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
//
// Whitebox unit tests for the Analyzer.
//
// The black-box suite (tests/run.sh) drives the compiled REPL and can only
// reach an analyzer technique when a real solve happens to route through it.
// Several intricate paths are hard to provoke that way -- the column-based
// Swordfish branch, the XY-chain "best chain" selection, simple coloring's
// Rule 2 contradiction (black-box only ever exercises Rule 4). These tests
// construct a candidate grid (or an analyzer result) directly and call the
// private Analyzer entry points through the AnalyzerTest friend, so one
// technique can be exercised on a position designed for it.
//
// Framework-free on purpose: this matches the project's no-dependency testing
// style. Each CHECK records a line; a nonzero exit code means a failure.

#include "board.h"
#include "analyzer.h"
#include "cell.h"
#include "coord.h"

#include <initializer_list>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

// The analyzer reads this application-global to decide whether to narrate its
// steps; it is normally defined in the REPL main (sudoku-solver.cpp), which this
// test binary does not link, so we supply it here. The tests keep it false.
bool sVerbose = false;

// One candidate XY-chain for the selection test: a value, a path, and how many
// cells it would eliminate. Declared here (Coord is public) so AnalyzerTest can
// turn it into the private Analyzer::XYChain.
struct XYSpec {
    int value;
    std::vector<Coord> chain;
    size_t num_elim;
};

// Friend hook: the only thing allowed to read/construct Analyzer internals.
struct AnalyzerTest {
    // --- swordfish ---
    static bool   find_swordfish(Analyzer &a, const Cell &c, const Value &v) { return a.find_swordfish(c, v); }
    static bool   act_on_swordfish(Analyzer &a)                              { return a.act_on_swordfish(); }
    static size_t swordfish_count(const Analyzer &a)                         { return a.mSwordfish.size(); }
    static bool   swordfish_row_based(const Analyzer &a)                     { return a.mSwordfish.at(0).is_row_based; }
    static Value  swordfish_value(const Analyzer &a)                         { return a.mSwordfish.at(0).value; }

    // --- xy-chain ---
    static bool   find_xychain(Analyzer &a, const Cell &c, const Value &v)   { return a.find_xychain(c, v); }
    static bool   act_on_xychain(Analyzer &a)                               { return a.act_on_xychain(); }
    static size_t xychain_count(const Analyzer &a)                          { return a.mXYChains.size(); }
    static Value  xychain_value(const Analyzer &a)                          { return a.mXYChains.begin()->value; }
    static size_t xychain_num_elim(const Analyzer &a)                       { return a.mXYChains.begin()->num_elim; }
    static size_t xychain_length(const Analyzer &a)                         { return a.mXYChains.begin()->chain.size(); }
    static Coord  xychain_front(const Analyzer &a)                          { return a.mXYChains.begin()->chain.front(); }
    static Coord  xychain_back(const Analyzer &a)                           { return a.mXYChains.begin()->chain.back(); }
    // Insert the candidate chains into a std::set<XYChain> exactly as
    // find_xychains does, and report the winner's (num_elim, length, value).
    static std::tuple<size_t,size_t,int> xychain_select_best(const std::vector<XYSpec> &specs) {
        std::set<Analyzer::XYChain> chains;
        for (auto const &sp : specs)
            chains.insert(Analyzer::XYChain{static_cast<Value>(sp.value), sp.chain, sp.num_elim});
        auto const &best = *chains.begin();
        return {best.num_elim, best.chain.size(), static_cast<int>(best.value)};
    }

    // --- y-wing ---
    static bool   find_ywing(Analyzer &a, const Cell &pivot)                                       { return a.find_ywing(pivot); }
    static bool   test_ywing(const Analyzer &a, const Cell &p, const Cell &w1, const Cell &w2, std::optional<Value> &out) { return a.test_ywing(p, w1, w2, out); }
    static bool   act_on_ywing(Analyzer &a)                                                        { return a.act_on_ywing(); }
    static size_t ywing_count(const Analyzer &a)                                                   { return a.mYWings.size(); }
    static Value  ywing_value(const Analyzer &a)                                                   { return a.mYWings.at(0).value; }

    // --- x-wing ---
    // find_xwing validates inline (like find_swordfish), so drive it on crafted
    // boards through its anchor entry point and inspect the recorded result.
    static bool   find_xwing(Analyzer &a, const Cell &c, const Value &v) { return a.find_xwing(c, v); }
    static bool   act_on_xwing(Analyzer &a)                              { return a.act_on_xwing(); }
    static size_t xwing_count(const Analyzer &a)                         { return a.mXWings.size(); }
    static bool   xwing_row_based(const Analyzer &a)                     { return a.mXWings.at(0).is_row_based; }
    static Value  xwing_value(const Analyzer &a)                         { return a.mXWings.at(0).value; }

    // --- simple coloring ---
    static bool test_color_chain(const Analyzer &a, const Analyzer::ColorChain &ch) { return a.test_color_chain(ch); }
    static bool act_on_color_chain(Analyzer &a)                                     { return a.act_on_color_chain(); }
    static void set_color_chain(Analyzer &a, const Analyzer::ColorChain &ch)        { a.mColorChains.clear(); a.mColorChains.push_back(ch); }
    static Analyzer::ColorChain make_color_chain(Value v, const std::vector<std::pair<Coord,bool>> &cells) {
        Analyzer::ColorChain ch;
        ch.value = v;
        for (auto const &cc : cells) ch.cells[cc.first] = cc.second;
        return ch;
    }
};

namespace {

int failures = 0;
void check(bool cond, const std::string &msg) {
    if (cond) { std::cout << "  ok    " << msg << "\n"; }
    else      { std::cout << "  FAIL  " << msg << "\n"; ++failures; }
}

// An empty 9x9 board: every cell a note carrying all nine candidates.
Board empty_board() { return Board(std::string(82, '.')); }

// Make cell (r,c) a note holding exactly the listed candidates (others struck).
void set_candidates(Board &board, size_t r, size_t c, std::initializer_list<int> keep) {
    for (int v = 1; v <= 9; ++v) {
        bool keep_it = false;
        for (int kv : keep) if (kv == v) { keep_it = true; break; }
        if (!keep_it) board.clear_note_at(r, c, static_cast<Value>(v));
    }
}

// Strike `value` everywhere except the listed (row,col) cells, so the technique
// under test sees a controlled distribution of that one candidate.
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

const Cell &cell_at(const Board &board, size_t r, size_t c) {
    return board.cells()[r * Board::width + c];
}

// ===========================================================================
// Swordfish
// ===========================================================================

// A column-based Swordfish on value 8.
//
//        c0 c1 c2 c3 c4 c5
//   r0    8  .  8  .  8  .      <- (0,4) is outside the base columns
//   r1    8  8  .  .  .  8      <- (1,5) is outside the base columns
//   r2    .  8  8  .  .  .
//
// Base columns {0,1,2}, each with two 8s; their rows union to exactly {0,1,2}.
// So 8 in those rows occupies the three base columns, and the strays at (0,4)
// and (1,5) are eliminable. Every cover row holds exactly three 8s (two in the
// pattern, one outside): the earlier ">3 candidates in the line" rule would
// have missed this, so this also locks the has_eliminations fix.
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
    bool found = AnalyzerTest::find_swordfish(analyzer, cell_at(board, 0, 0), V);
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
    check(has_candidate(board, 0, 0, V) && has_candidate(board, 1, 0, V)
       && has_candidate(board, 1, 1, V) && has_candidate(board, 2, 1, V)
       && has_candidate(board, 0, 2, V) && has_candidate(board, 2, 2, V),
          "all six pattern cells kept candidate 8");
}

// Same base columns, strays removed: a Swordfish shape with nothing to
// eliminate must not be recorded (recording it would assert in act).
void test_swordfish_no_elimination() {
    std::cout << "[swordfish] a pattern with nothing to eliminate is not recorded\n";
    Board board = empty_board();
    const Value V = kEight;
    confine_value(board, V, { {0,0},{1,0}, {1,1},{2,1}, {0,2},{2,2} });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_swordfish(analyzer, cell_at(board, 0, 0), V);
    check(!found, "no Swordfish reported when there is nothing to eliminate");
    check(AnalyzerTest::swordfish_count(analyzer) == 0, "no Swordfish recorded");
}

// ===========================================================================
// XY-chain
// ===========================================================================

// A length-3 chain in row 0:  (0,0){1,2} - (0,1){2,3} - (0,2){1,3}.
// Following value 1: (0,0) 1->2, (0,1) 2->3, (0,2) 3->1, back to 1. So a cell
// seeing both ends with candidate 1 can drop it. Candidate 1 is confined to the
// two ends and one target (0,3){1,4}, so exactly one elimination results.
void test_xychain_detect_and_act() {
    std::cout << "[xy-chain] detection and elimination on a length-3 chain\n";
    Board board = empty_board();
    set_candidates(board, 0, 0, {1, 2});
    set_candidates(board, 0, 1, {2, 3});
    set_candidates(board, 0, 2, {1, 3});
    set_candidates(board, 0, 3, {1, 4});   // sees both ends via row 0; the target
    confine_value(board, kOne, { {0,0}, {0,2}, {0,3} });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_xychain(analyzer, cell_at(board, 0, 0), kOne);
    check(found, "XY-chain detected starting from (0,0) on value 1");
    check(AnalyzerTest::xychain_count(analyzer) == 1, "exactly one chain retained");
    if (AnalyzerTest::xychain_count(analyzer) == 1) {
        check(AnalyzerTest::xychain_value(analyzer) == kOne, "chain value is 1");
        check(AnalyzerTest::xychain_length(analyzer) == 3, "chain has length 3");
        check(AnalyzerTest::xychain_num_elim(analyzer) == 1, "chain counts exactly one elimination");
        check(AnalyzerTest::xychain_front(analyzer) == Coord(0,0)
           && AnalyzerTest::xychain_back(analyzer) == Coord(0,2), "chain runs (0,0)..(0,2)");
    }

    bool acted = AnalyzerTest::act_on_xychain(analyzer);
    check(acted, "act_on_xychain reports an elimination");
    check(!has_candidate(board, 0, 3, kOne), "candidate 1 eliminated from (0,3)");
    check(has_candidate(board, 0, 0, kOne) && has_candidate(board, 0, 2, kOne),
          "chain-end cells kept candidate 1");
}

// The selection invariant: among candidate chains, the kept one has the most
// eliminations, ties broken by the shorter chain. This is the ordering the
// analyzer relies on (std::set<XYChain> begin() == most desirable).
void test_xychain_best_selection() {
    std::cout << "[xy-chain] the most-eliminations / shortest chain is selected\n";
    auto mkchain = [](std::initializer_list<std::pair<size_t,size_t>> cs) {
        std::vector<Coord> v;
        for (auto const &c : cs) v.emplace_back(c.first, c.second);
        return v;
    };
    std::vector<XYSpec> specs = {
        {1, mkchain({{0,0},{0,1},{0,2},{0,3}}), 2},  // 2 elim, length 4
        {2, mkchain({{1,0},{1,1},{1,2}}),       1},  // 1 elim, length 3
        {3, mkchain({{2,0},{2,1},{2,2}}),       2},  // 2 elim, length 3  <- best
    };
    auto [num_elim, length, value] = AnalyzerTest::xychain_select_best(specs);
    check(num_elim == 2, "winner has the most eliminations");
    check(length == 3,   "ties broken toward the shorter chain");
    check(value == 3,    "winner is the 2-elim length-3 chain");
}

// ===========================================================================
// Y-wing
// ===========================================================================

// Pivot (0,0){1,2}; wings (0,1){1,3} (shares 1) and (1,0){2,3} (shares 2); both
// wings carry 3. (1,1) sees both wings and carries 3, so it is eliminated.
void test_ywing_detect_and_act() {
    std::cout << "[y-wing] detection and elimination of the shared wing value\n";
    Board board = empty_board();
    set_candidates(board, 0, 0, {1, 2});   // pivot
    set_candidates(board, 0, 1, {1, 3});   // wing sharing 1
    set_candidates(board, 1, 0, {2, 3});   // wing sharing 2
    set_candidates(board, 1, 1, {3, 4});   // sees both wings; the target
    confine_value(board, kThree, { {0,1}, {1,0}, {1,1} });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_ywing(analyzer, cell_at(board, 0, 0));
    check(found, "Y-wing detected with pivot (0,0)");
    check(AnalyzerTest::ywing_count(analyzer) == 1, "exactly one Y-wing recorded");
    if (AnalyzerTest::ywing_count(analyzer) == 1)
        check(AnalyzerTest::ywing_value(analyzer) == kThree, "elimination value is 3");

    bool acted = AnalyzerTest::act_on_ywing(analyzer);
    check(acted, "act_on_ywing reports an elimination");
    check(!has_candidate(board, 1, 1, kThree), "candidate 3 eliminated from (1,1)");
    check(has_candidate(board, 0, 1, kThree) && has_candidate(board, 1, 0, kThree),
          "wing cells kept candidate 3");
}

// test_ywing must reject near-misses: wings sharing the *same* value with the
// pivot, and wings whose non-shared values differ.
void test_ywing_rejects_non_patterns() {
    std::cout << "[y-wing] the predicate rejects near-miss configurations\n";
    Board board = empty_board();
    set_candidates(board, 0, 0, {1, 2});   // pivot
    set_candidates(board, 0, 1, {1, 3});   // shares 1, other 3
    set_candidates(board, 1, 0, {1, 4});   // shares 1 too (same as the other wing)
    set_candidates(board, 2, 0, {2, 5});   // shares 2, other 5 (!= 3)

    Analyzer analyzer(board);
    std::optional<Value> out;  // out-param; only engaged when test_ywing returns true
    bool both_share_one = AnalyzerTest::test_ywing(analyzer,
        cell_at(board, 0, 0), cell_at(board, 0, 1), cell_at(board, 1, 0), out);
    check(!both_share_one, "rejected: both wings share the same value with the pivot");

    bool others_differ = AnalyzerTest::test_ywing(analyzer,
        cell_at(board, 0, 0), cell_at(board, 0, 1), cell_at(board, 2, 0), out);
    check(!others_differ, "rejected: the wings' non-shared values differ");
}

// ===========================================================================
// X-Wing
// ===========================================================================

// find_xwing validates inline. The black-box suite drives it on real solves;
// these whitebox cases craft small boards and drive find_xwing through its
// anchor entry point -- the same way the Swordfish tests drive find_swordfish --
// to cover both orientations and the rejection paths a happy-path solve does not
// isolate.

// A row-based X-Wing on value 7: rows 0 and 3 each hold 7 in exactly columns 1
// and 5. Columns 1 and 5 carry one extra 7 apiece (rows 6 and 7), so the pattern
// is actionable and those strays are eliminated.
//
//        c1 c5
//   r0    7  7      <- top corners
//   r3    7  7      <- bottom corners
//   r6    7         <- stray cleared from column 1
//   r7       7      <- stray cleared from column 5
void test_xwing_row_based() {
    std::cout << "[x-wing] row-based detection, action\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5}, {6,1}, {7,5} });

    Analyzer analyzer(board);
    // Anchor on (0,1), the first 7 of row 0 -- find_xwing's top-left corner.
    bool found = AnalyzerTest::find_xwing(analyzer, cell_at(board, 0, 1), V);
    check(found, "row X-Wing detected with anchor (0,1)");
    check(AnalyzerTest::xwing_count(analyzer) == 1, "exactly one X-Wing recorded");
    if (AnalyzerTest::xwing_count(analyzer) == 1) {
        check(AnalyzerTest::xwing_row_based(analyzer), "recorded X-Wing is row-based");
        check(AnalyzerTest::xwing_value(analyzer) == V, "recorded X-Wing is for value 7");
    }

    bool acted = AnalyzerTest::act_on_xwing(analyzer);
    check(acted, "act_on_xwing reports an elimination");
    check(!has_candidate(board, 6, 1, V), "stray 7 at (6,1) eliminated from column 1");
    check(!has_candidate(board, 7, 5, V), "stray 7 at (7,5) eliminated from column 5");
    check(has_candidate(board, 0, 1, V) && has_candidate(board, 0, 5, V)
       && has_candidate(board, 3, 1, V) && has_candidate(board, 3, 5, V),
          "all four corner cells kept candidate 7");
}

// The transpose: a column-based X-Wing on value 7. Columns 0 and 3 each hold 7
// in exactly rows 1 and 5; rows 1 and 5 carry one extra 7 apiece to eliminate.
// Anchoring on (1,0) makes the row search bail (row 1 has three 7s) before the
// column search succeeds, so this exercises the column orientation.
//
//        c0 c3 c6 c7
//   r1    7  7  7        <- left/right corners; c6 stray cleared from row 1
//   r5    7  7     7     <- left/right corners; c7 stray cleared from row 5
void test_xwing_column_based() {
    std::cout << "[x-wing] column-based detection, action\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {1,0},{5,0}, {1,3},{5,3}, {1,6}, {5,7} });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_xwing(analyzer, cell_at(board, 1, 0), V);
    check(found, "column X-Wing detected with anchor (1,0)");
    check(AnalyzerTest::xwing_count(analyzer) == 1, "exactly one X-Wing recorded");
    if (AnalyzerTest::xwing_count(analyzer) == 1) {
        check(!AnalyzerTest::xwing_row_based(analyzer), "recorded X-Wing is column-based");
        check(AnalyzerTest::xwing_value(analyzer) == V, "recorded X-Wing is for value 7");
    }

    bool acted = AnalyzerTest::act_on_xwing(analyzer);
    check(acted, "act_on_xwing reports an elimination");
    check(!has_candidate(board, 1, 6, V), "stray 7 at (1,6) eliminated from row 1");
    check(!has_candidate(board, 5, 7, V), "stray 7 at (5,7) eliminated from row 5");
    check(has_candidate(board, 1, 0, V) && has_candidate(board, 5, 0, V)
       && has_candidate(board, 1, 3, V) && has_candidate(board, 5, 3, V),
          "all four corner cells kept candidate 7");
}

// A perfect rectangle with nothing to eliminate must not be recorded (recording
// it would assert in act). Rows 0 and 3 hold 7 in columns 1 and 5 and nowhere
// else, so neither column carries a third candidate.
void test_xwing_no_elimination() {
    std::cout << "[x-wing] a rectangle with nothing to eliminate is not recorded\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5} });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_xwing(analyzer, cell_at(board, 0, 1), V);
    check(!found, "no X-Wing reported when there is nothing to eliminate");
    check(AnalyzerTest::xwing_count(analyzer) == 0, "no X-Wing recorded");
}

// A near-miss: the two candidate rows do not share both columns, so no rectangle
// forms. Row 0 holds 7 in columns 1 and 5; row 3 in columns 1 and 8 -- they
// share column 1 only, so the fourth corner is absent and find_xwing reports
// nothing from either orientation.
void test_xwing_misaligned_not_found() {
    std::cout << "[x-wing] misaligned candidate lines yield no pattern\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,8}, {7,5} });

    Analyzer analyzer(board);
    bool found = AnalyzerTest::find_xwing(analyzer, cell_at(board, 0, 1), V);
    check(!found, "no X-Wing reported when the candidate lines are misaligned");
    check(AnalyzerTest::xwing_count(analyzer) == 0, "no X-Wing recorded");
}

// ===========================================================================
// Simple coloring
// ===========================================================================

// Rule 2 (the contradiction): two same-colored cells in one unit mean that
// whole color is false. Black-box solves only ever exercise Rule 4, so this is
// the path that otherwise goes untested. Two green cells share row 0, so both
// greens drop the value; the lone red is untouched.
void test_colorchain_rule2_contradiction() {
    std::cout << "[simple coloring] Rule 2 eliminates a color that repeats in a unit\n";
    Board board = empty_board();
    const Value V = kFive;
    confine_value(board, V, { {0,0}, {0,4}, {8,8} });

    auto chain = AnalyzerTest::make_color_chain(V, {
        { Coord(0,0), true  },   // green
        { Coord(0,4), true  },   // green -- same row as the other green
        { Coord(8,8), false },   // red
    });

    Analyzer analyzer(board);
    check(AnalyzerTest::test_color_chain(analyzer, chain),
          "test_color_chain reports a same-color-in-unit chain as actionable");

    AnalyzerTest::set_color_chain(analyzer, chain);
    bool acted = AnalyzerTest::act_on_color_chain(analyzer);
    check(acted, "act_on_color_chain reports an elimination");
    check(!has_candidate(board, 0, 0, V) && !has_candidate(board, 0, 4, V),
          "both green cells (the repeated color) dropped the value");
    check(has_candidate(board, 8, 8, V), "the lone red cell is untouched");
}

// A consistent chain with no same-color conflict and no off-chain cell seeing
// both colors is not actionable.
void test_colorchain_benign_not_actionable() {
    std::cout << "[simple coloring] a conflict-free chain with nothing to see is inert\n";
    Board board = empty_board();
    const Value V = kFive;
    confine_value(board, V, { {0,0}, {4,4} });   // value lives only on the chain

    auto chain = AnalyzerTest::make_color_chain(V, {
        { Coord(0,0), true  },   // green
        { Coord(4,4), false },   // red -- shares no unit with the green
    });

    Analyzer analyzer(board);
    check(!AnalyzerTest::test_color_chain(analyzer, chain),
          "test_color_chain reports a benign chain as not actionable");
}

} // namespace

int main() {
    test_swordfish_column_based();
    test_swordfish_no_elimination();
    test_xychain_detect_and_act();
    test_xychain_best_selection();
    test_ywing_detect_and_act();
    test_ywing_rejects_non_patterns();
    test_xwing_row_based();
    test_xwing_column_based();
    test_xwing_no_elimination();
    test_xwing_misaligned_not_found();
    test_colorchain_rule2_contradiction();
    test_colorchain_benign_not_actionable();

    std::cout << "----------------------------------------\n";
    if (failures == 0) { std::cout << "unit: all checks passed\n"; return 0; }
    std::cout << "unit: " << failures << " check(s) failed\n";
    return 1;
}
