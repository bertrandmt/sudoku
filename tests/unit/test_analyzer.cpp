// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
//
// Whitebox unit tests for the Analyzer.
//
// The black-box suite (tests/run.sh) drives the compiled REPL and can only
// reach an analyzer technique when a real solve happens to route through it.
// Several intricate paths are hard to provoke that way: a solve exercises only
// whichever path its position happens to take, leaving both a technique's
// rejections and its less-travelled acceptances dark. The Swordfish
// no-eliminations rejection is the first kind; the XY-chain "best chain"
// selection and simple coloring's Rule 2 contradiction (black-box only ever
// exercises Rule 4) are the second.
// These tests construct a candidate grid (or an analyzer result) directly and
// drive one technique's entry points, on a position designed for it. Every
// technique reaches those entry points through a public static seam, so the
// AnalyzerTest friend below exists only for the rebinding-ctor regression test,
// which is about the Analyzer itself rather than any technique.
//
// Framework-free on purpose: this matches the project's no-dependency testing
// style. Each CHECK records a line; a nonzero exit code means a failure.

#include "board.h"
#include "analyzer.h"
#include "analyzer-nakedpairs.h"
#include "analyzer-hiddenpairs.h"
#include "analyzer-xwing.h"
#include "analyzer-colorchain.h"
#include "analyzer-ywing.h"
#include "analyzer-swordfish.h"
#include "analyzer-finnedxwing.h"
#include "analyzer-finnedswordfish.h"
#include "analyzer-xychain.h"
#include "cell.h"
#include "coord.h"

#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// The analyzer reads this application-global to decide whether to narrate its
// steps; it is normally defined in the REPL main (sudoku-solver.cpp), which this
// test binary does not link, so we supply it here. The tests keep it false.
bool sVerbose = false;

// Friend hook: the only thing allowed to read/construct Analyzer internals.
struct AnalyzerTest {
    // --- rebinding ctor / carried findings (issue #7) ---
    // These reach the single per-state member that survives the state copy, so
    // the regression test can prove the rebinding ctor carries it forward
    // without naming any (file-local) concrete Finding subtype: it downcasts
    // nothing and compares via Finding::print instead.
    static size_t findings_bucket_count(const Analyzer &a) { return a.mFindings.size(); }
    static size_t findings_total(const Analyzer &a) {
        size_t n = 0;
        for (auto const &bucket : a.mFindings) n += bucket.size();
        return n;
    }
    static std::string findings_render(const Analyzer &a) {
        std::ostringstream os;
        for (auto const &bucket : a.mFindings)
            for (auto const &f : bucket) { f->print(os); os << ";"; }
        return os.str();
    }

    // --- xy-chain ---
    // No hooks: XY is a standalone Technique. It is materialized-object shaped
    // (see docs/test-predicate-idiom.md), but unlike simple coloring its cases
    // do not drive its scoring predicate; they drive the per-anchor search
    // XYChainTechnique::find_xychain and the best-chain selection
    // XYChainTechnique::record_if_best, both public statics, and read the
    // recorded XYChainFinding (visible via analyzer-xychain.h). No friendship.

    // --- y-wing ---
    // No hooks: YW is a standalone Technique. It is given-tuple shaped (see
    // docs/test-predicate-idiom.md), so its validation predicate is a public
    // static YWingTechnique::test_ywing the cases call directly on crafted
    // near-misses. Because those cases also drive find_ywing on a crafted pivot
    // and read the recorded finding's value, find_ywing is likewise a public
    // static and YWingFinding is declared in analyzer-ywing.h -- both without
    // friendship.

    // --- the fish / naked pair / hidden pair ---
    // No hooks: every one of them is a standalone Technique. Naked pair and
    // hidden pair are given-tuple shaped, so their test_naked_pair /
    // test_hidden_pair are public statics the whitebox cases call directly. The
    // fish -- plain X-Wing and Swordfish, and their finned variants -- are all
    // scan-fused (no test_ predicate -- see docs/test-predicate-idiom.md); the
    // cases drive each one's per-anchor entry (XWingTechnique::find_xwing,
    // SwordfishTechnique::find_swordfish,
    // FinnedXWingTechnique::find_finned_xwing,
    // FinnedSwordfishTechnique::find_finned_swordfish) on crafted boards and
    // inspect the recorded finding, which each fish declares in its own header
    // for that reason. Either way: no friendship.

    // --- simple coloring ---
    // No hooks: SC is a standalone Technique. It is materialized-object shaped
    // (see docs/test-predicate-idiom.md), so its validation predicate is a
    // public static ColorChainTechnique::test_color_chain(board, chain) the
    // cases call directly, and ColorChainFinding is declared in
    // analyzer-colorchain.h so a case can build a chain and drive apply() --
    // both without friendship.
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

// The lone finding recorded in `out`, downcast to `F`; nullptr if the search
// recorded something other than exactly one finding, or one of another type.
// Used by the cases that read a lone recorded finding's concrete fields. Why
// there is only one differs: X-Wing and Swordfish stop at the first hit, XY-chain
// retains only the most desirable chain, and Y-Wing records every pattern it
// finds but its case crafts a board bearing exactly one. Not every hooked
// technique wants it -- simple coloring's cases judge chains they build
// themselves through the promoted test_color_chain static, and never downcast a
// bucket. The entry is an `F` by construction, for the reason technique.h's
// bucket_cast documents; this is the test-side counterpart, differing on purpose
// in what it does when the invariant breaks -- its result is check()ed at the
// call site, so a wrong type fails loudly as a failed check rather than aborting
// the suite or silently skipping the field assertions.
template<class F>
const F *only(const FindingList &out) {
    if (out.size() != 1) return nullptr;
    return dynamic_cast<const F *>(out.front().get());
}

// ===========================================================================
// Swordfish
// ===========================================================================

// Swordfish is scan-fused (no test_ predicate -- see docs/test-predicate-idiom.md),
// like its sibling X-Wing. These cases craft small boards and drive
// SwordfishTechnique::find_swordfish through its anchor entry point, then read
// the recorded SwordfishFinding's fields directly (the finding type is visible
// via analyzer-swordfish.h -- no friendship needed).

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

    SwordfishTechnique sf;  // needed for apply() below; find_swordfish is static
    FindingList found;
    check(SwordfishTechnique::find_swordfish(board, cell_at(board, 0, 0), V, found),
          "column Swordfish detected on a position with only tight cover lines");
    check(found.size() == 1, "exactly one Swordfish recorded");
    auto const *f = only<SwordfishFinding>(found);
    check(f, "the recorded finding is a SwordfishFinding");
    if (f) {
        check(!f->is_row_based, "recorded Swordfish is column-based");
        check(f->value == V, "recorded Swordfish is for value 8");
    }

    bool acted = sf.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 0, 4, V), "stray 8 at (0,4) eliminated");
    check(!has_candidate(board, 1, 5, V), "stray 8 at (1,5) eliminated");
    check(has_candidate(board, 0, 0, V) && has_candidate(board, 1, 0, V)
       && has_candidate(board, 1, 1, V) && has_candidate(board, 2, 1, V)
       && has_candidate(board, 0, 2, V) && has_candidate(board, 2, 2, V),
          "all six pattern cells kept candidate 8");
}

// A row-based Swordfish on value 8 -- the transpose of the column-based case
// above. The act side reaches the row-based branch (eliminate from columns)
// only here in the whitebox tests; the integration suite drives it, but this
// locks it in as a regression guard.
//
//        c0 c1 c2
//   r0    8  8  .
//   r1    .  8  8
//   r2    8  .  8
//   r4    8  .  .      <- (4,0) is outside the base rows
//   r5    .  8  .      <- (5,1) is outside the base rows
//
// Base rows {0,1,2}, each with two 8s; their columns union to exactly {0,1,2}.
// So 8 in those columns occupies the three base rows, and the strays at (4,0)
// and (5,1) are eliminable. As with the column-based case, every cover column
// holds exactly three 8s (two in the pattern, one outside): a tight cover line.
void test_swordfish_row_based() {
    std::cout << "[swordfish] row-based detection and action (eliminate from columns)\n";
    Board board = empty_board();
    const Value V = kEight;
    confine_value(board, V, {
        {0,0},{0,1},        // row 0: columns {0,1}
        {1,1},{1,2},        // row 1: columns {1,2}
        {2,0},{2,2},        // row 2: columns {0,2}
        {4,0},{5,1},        // strays to be eliminated
    });

    SwordfishTechnique sf;  // needed for apply() below; find_swordfish is static
    FindingList found;
    check(SwordfishTechnique::find_swordfish(board, cell_at(board, 0, 0), V, found),
          "row Swordfish detected on a position with only tight cover lines");
    check(found.size() == 1, "exactly one Swordfish recorded");
    auto const *f = only<SwordfishFinding>(found);
    check(f, "the recorded finding is a SwordfishFinding");
    if (f) {
        check(f->is_row_based, "recorded Swordfish is row-based");
        check(f->value == V, "recorded Swordfish is for value 8");
    }

    bool acted = sf.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 4, 0, V), "stray 8 at (4,0) eliminated");
    check(!has_candidate(board, 5, 1, V), "stray 8 at (5,1) eliminated");
    check(has_candidate(board, 0, 0, V) && has_candidate(board, 0, 1, V)
       && has_candidate(board, 1, 1, V) && has_candidate(board, 1, 2, V)
       && has_candidate(board, 2, 0, V) && has_candidate(board, 2, 2, V),
          "all six pattern cells kept candidate 8");
}

// Same base columns, strays removed: a Swordfish shape with nothing to
// eliminate must not be recorded (recording it would assert in act).
void test_swordfish_no_elimination() {
    std::cout << "[swordfish] a pattern with nothing to eliminate is not recorded\n";
    Board board = empty_board();
    const Value V = kEight;
    confine_value(board, V, { {0,0},{1,0}, {1,1},{2,1}, {0,2},{2,2} });

    FindingList found;
    check(!SwordfishTechnique::find_swordfish(board, cell_at(board, 0, 0), V, found),
          "no Swordfish reported when there is nothing to eliminate");
    check(found.empty(), "no Swordfish recorded");
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

    XYChainTechnique xy;  // needed for apply() below; find_xychain is static
    FindingList found;
    check(XYChainTechnique::find_xychain(board, cell_at(board, 0, 0), kOne, found),
          "XY-chain detected starting from (0,0) on value 1");
    check(found.size() == 1, "exactly one chain retained");
    auto const *f = only<XYChainFinding>(found);
    check(f, "the recorded finding is an XYChainFinding");
    if (f) {
        check(f->value == kOne, "chain value is 1");
        check(f->chain.size() == 3, "chain has length 3");
        check(f->num_elim == 1, "chain counts exactly one elimination");
        check(f->chain.front() == Coord(0,0) && f->chain.back() == Coord(0,2),
              "chain runs (0,0)..(0,2)");
    }

    bool acted = xy.apply(board, found);
    check(acted, "apply reports an elimination");
    check(found.empty(), "the applied chain is consumed");
    check(!has_candidate(board, 0, 3, kOne), "candidate 1 eliminated from (0,3)");
    check(has_candidate(board, 0, 0, kOne) && has_candidate(board, 0, 2, kOne),
          "chain-end cells kept candidate 1");
}

// The selection invariant: of the chains offered to a bucket, the one retained
// has the most eliminations, ties broken by the shorter chain, and a chain that
// is merely as good as the incumbent does not displace it. Offered to
// record_if_best directly rather than found on a board: it takes several
// competing chains to exercise, and which chains a crafted board yields is not
// something the case can dictate.
void test_xychain_best_selection() {
    std::cout << "[xy-chain] the most-eliminations / shortest chain is retained\n";
    auto mkchain = [](std::initializer_list<std::pair<size_t,size_t>> cs) {
        std::vector<Coord> v;
        for (auto const &c : cs) v.emplace_back(c.first, c.second);
        return v;
    };

    FindingList bucket;
    check(XYChainTechnique::record_if_best(bucket, {kOne, mkchain({{0,0},{0,1},{0,2},{0,3}}), 2}),
          "the first chain offered is recorded");                     // 2 elim, length 4
    check(!XYChainTechnique::record_if_best(bucket, {kTwo, mkchain({{1,0},{1,1},{1,2}}), 1}),
          "a chain with fewer eliminations is rejected");             // 1 elim, length 3
    check(XYChainTechnique::record_if_best(bucket, {kThree, mkchain({{2,0},{2,1},{2,2}}), 2}),
          "an equal-elimination shorter chain displaces it");         // 2 elim, length 3 <- best

    // Isolate the two remaining rejection gates against that incumbent. The
    // first would be strictly better on its own terms (three eliminations beats
    // two), so only the same-value-same-endpoints dedup can reject it; the
    // second is neither better nor worse, so only the strict-improvement rule
    // can.
    check(!XYChainTechnique::record_if_best(bucket, {kThree, mkchain({{2,0},{3,1},{2,2}}), 3}),
          "a rediscovered chain -- same value and endpoints -- is rejected even with more eliminations");
    check(!XYChainTechnique::record_if_best(bucket, {kFour, mkchain({{4,0},{4,1},{4,2}}), 2}),
          "an equally desirable but distinct chain does not displace the incumbent");

    auto const *best = only<XYChainFinding>(bucket);
    check(best, "the bucket holds exactly one XYChainFinding");
    if (best) {
        check(best->num_elim == 2,     "winner has the most eliminations");
        check(best->chain.size() == 3, "ties broken toward the shorter chain");
        check(best->value == kThree,   "winner is the 2-elim length-3 chain");
    }
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

    FindingList findings;
    bool found = YWingTechnique::find_ywing(board, cell_at(board, 0, 0), findings);
    check(found, "Y-wing detected with pivot (0,0)");
    check(findings.size() == 1, "exactly one Y-wing recorded");
    auto const *yw = only<YWingFinding>(findings);
    check(yw, "the recorded finding is a YWingFinding");
    if (yw) check(yw->value == kThree, "elimination value is 3");

    YWingTechnique tech;
    bool acted = tech.apply(board, findings);
    check(acted, "apply reports an elimination");
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

    std::optional<Value> out;  // out-param; only engaged when test_ywing returns true
    bool both_share_one = YWingTechnique::test_ywing(board,
        cell_at(board, 0, 0), cell_at(board, 0, 1), cell_at(board, 1, 0), out);
    check(!both_share_one, "rejected: both wings share the same value with the pivot");

    bool others_differ = YWingTechnique::test_ywing(board,
        cell_at(board, 0, 0), cell_at(board, 0, 1), cell_at(board, 2, 0), out);
    check(!others_differ, "rejected: the wings' non-shared values differ");
}

// ===========================================================================
// Notes set operations
// ===========================================================================

// The bitmask set primitives the analyzers now lean on (#28). Tested directly,
// not just through their call sites, so a regression names the primitive.
void test_notes_set_ops() {
    std::cout << "[notes] bitmask set operations: ==, intersects, shared_value\n";
    Notes a; a.clear(); a.set(kThree, true); a.set(kFive, true);   // {3,5}
    Notes b; b.clear(); b.set(kFive, true);  b.set(kThree, true);  // {3,5}, set reversed
    Notes c; c.clear(); c.set(kThree, true); c.set(kSix, true);    // {3,6}, overlaps a on 3
    Notes d; d.clear(); d.set(kOne, true);   d.set(kTwo, true);    // {1,2}, disjoint from a

    // operator== is set equality on the mask: order-insensitive, both directions.
    check(a == b && b == a, "equal: same candidates regardless of set order");
    check(!(a == c), "unequal: candidate sets {3,5} and {3,6} differ");

    // intersects: zero shared is a legal answer (no assert), unlike shared_value.
    check(a.intersects(c), "intersects: {3,5} and {3,6} share one (3)");
    check(a.intersects(b), "intersects: {3,5} and {3,5} share two (3 and 5)");
    check(!a.intersects(d), "disjoint: {3,5} and {1,2} share nothing");

    // shared_value: the single common candidate.
    check(a.shared_value(c) == kThree, "shared_value: {3,5} and {3,6} share 3");
}

// ===========================================================================
// Naked Pair
// ===========================================================================

// test_naked_pair is the archetypal given-tuple predicate (see
// docs/test-predicate-idiom.md): find_ enumerates cell pairs, the predicate
// judges each. The black-box suite only reaches it on a real solve that happens
// to route through a naked pair; these whitebox cases hand it crafted tuples to
// pin down its branches directly.
//
// test_naked_pair is a composite: it accepts only a genuine pair (Notes::==)
// that is *also* actionable (would_act). Those two gates are tested separately
// below so a would_act change surfaces as an actionability failure, not a
// phantom pair-match regression.
void test_naked_pair_accept_and_reject() {
    std::cout << "[naked pair] the pair-match and actionability gates\n";

    // --- pair-match gate ---
    // Every other cell in the row carries all nine candidates, so any genuine
    // pair here is trivially actionable; this isolates the pair-match decision.
    Board board = empty_board();
    set_candidates(board, 0, 0, {3, 5});   // the pair...
    set_candidates(board, 0, 1, {3, 5});   // ...its twin in the same row
    set_candidates(board, 0, 2, {3, 6});   // shares only one value -- not a pair

    // The predicate is a promoted public static (no friend hook): call it directly,
    // instantiated on the cells' shared Row.
    check(NakedPairTechnique::test_naked_pair(cell_at(board, 0, 0), cell_at(board, 0, 1), board.row(cell_at(board, 0, 0))),
          "accepted: two cells holding the same candidate pair {3,5}");
    // Reject short-circuits on the set compare, before would_act: a pure
    // pair-match check, immune to changes in actionability logic.
    check(!NakedPairTechnique::test_naked_pair(cell_at(board, 0, 0), cell_at(board, 0, 2), board.row(cell_at(board, 0, 0))),
          "rejected: candidate sets {3,5} and {3,6} differ");

    // --- actionability gate (would_act) ---
    // Same shape of matching pair, but its two values live nowhere else in the
    // row, so there is nothing to eliminate and the predicate must decline. This
    // pins the would_act gate explicitly rather than leaning on it implicitly.
    Board inert = empty_board();
    set_candidates(inert, 0, 0, {7, 8});
    set_candidates(inert, 0, 1, {7, 8});
    confine_value(inert, kSeven, { {0, 0}, {0, 1} });
    confine_value(inert, kEight, { {0, 0}, {0, 1} });

    check(!NakedPairTechnique::test_naked_pair(cell_at(inert, 0, 0), cell_at(inert, 0, 1), inert.row(cell_at(inert, 0, 0))),
          "rejected: a real pair with nothing to act on (would_act gate)");
}

// ===========================================================================
// Hidden pair
// ===========================================================================

// test_hidden_pair is given-tuple shaped like test_naked_pair (see
// docs/test-predicate-idiom.md): find_ enumerates the partner cell, the
// predicate judges each (c1,c2,v1,v2). The predicate is a public static, so
// these cases call it directly -- no friend hook. They pin down its substantive
// branches -- value ordering, cell ordering, the hidden loop, actionability,
// and partner incompleteness -- and leave the trivial distinctness/shape guards
// (c1 == c2, v1 == v2, set membership, isNote, c1 failing to carry the pair)
// untested, as the test_naked_pair sibling does. Most of these branches are
// also hit incidentally by any black-box solve, since find_hidden_pair feeds
// every unfiltered cell to the predicate; the one find_ genuinely cannot reach
// is value ordering
// (HiddenPairTechnique::find enumerates pv2 = pv1 + 1, so v1 < v2 always holds), which
// is the load-bearing reason to test the predicate directly rather than only
// through find_.
void test_hidden_pair_accept_and_reject() {
    std::cout << "[hidden pair] the hidden, actionability, ordering and partner gates\n";

    // --- "hidden" gate ---
    // (0,0) and (0,1) carry {3,5} among all nine candidates; 3 and 5 live
    // nowhere else, so the pair is genuinely hidden and actionable (each cell
    // still holds seven other candidates to strip).
    Board board = empty_board();
    confine_value(board, kThree, { {0, 0}, {0, 1} });
    confine_value(board, kFive,  { {0, 0}, {0, 1} });

    check(HiddenPairTechnique::test_hidden_pair(cell_at(board, 0, 0), cell_at(board, 0, 1), kThree, kFive, board.row(cell_at(board, 0, 0))),
          "accepted: {3,5} confined to two cells, each with more to strip");

    // A third cell in the row carrying just one of the values breaks "hidden".
    Board stray = empty_board();
    confine_value(stray, kThree, { {0, 0}, {0, 1}, {0, 2} });
    confine_value(stray, kFive,  { {0, 0}, {0, 1} });

    check(!HiddenPairTechnique::test_hidden_pair(cell_at(stray, 0, 0), cell_at(stray, 0, 1), kThree, kFive, stray.row(cell_at(stray, 0, 0))),
          "rejected: a third cell carries 3, so {3,5} is not hidden");

    // --- actionability gate ---
    // Same hidden {3,5}, but both cells are bivalue: that is a naked pair, with
    // nothing for hidden pair to eliminate.
    Board naked = empty_board();
    set_candidates(naked, 0, 0, {3, 5});
    set_candidates(naked, 0, 1, {3, 5});
    confine_value(naked, kThree, { {0, 0}, {0, 1} });
    confine_value(naked, kFive,  { {0, 0}, {0, 1} });

    check(!HiddenPairTechnique::test_hidden_pair(cell_at(naked, 0, 0), cell_at(naked, 0, 1), kThree, kFive, naked.row(cell_at(naked, 0, 0))),
          "rejected: both cells bivalue {3,5} -- a naked pair, nothing to strip");

    // --- ordering and partner gates (reuse the accepting board) ---
    // Cell ordering: c2 must come after c1.
    check(!HiddenPairTechnique::test_hidden_pair(cell_at(board, 0, 1), cell_at(board, 0, 0), kThree, kFive, board.row(cell_at(board, 0, 0))),
          "rejected: cells passed out of order (c2 before c1)");

    // Value ordering: v2 must come after v1. find_ guarantees this; a direct
    // test caller can violate it, so the guard must be tested by the direct call.
    check(!HiddenPairTechnique::test_hidden_pair(cell_at(board, 0, 0), cell_at(board, 0, 1), kFive, kThree, board.row(cell_at(board, 0, 0))),
          "rejected: values passed out of order (v2 < v1)");

    // Partner incompleteness: c2 carries only one of the two values. find_ does
    // reach this branch (it hands every unfiltered cell to the predicate). Both
    // values are confined to the pair so the hidden loop finds nothing: the carry
    // gate is then the *only* thing that can reject, so deleting it would flip the
    // case to accept -- isolating this gate rather than letting the hidden loop
    // mask it.
    Board partial = empty_board();
    confine_value(partial, kThree, { {0, 0}, {0, 1} });
    confine_value(partial, kFive,  { {0, 0} });   // 5 lives only in (0,0)
    set_candidates(partial, 0, 1, {3});           // partner carries 3 but not 5

    check(!HiddenPairTechnique::test_hidden_pair(cell_at(partial, 0, 0), cell_at(partial, 0, 1), kThree, kFive, partial.row(cell_at(partial, 0, 0))),
          "rejected: partner carries 3 but not 5");
}

// ===========================================================================
// X-Wing
// ===========================================================================

// X-Wing is scan-fused (no test_ predicate -- see docs/test-predicate-idiom.md).
// The black-box suite drives it on real solves; these whitebox cases craft small
// boards and drive XWingTechnique::find_xwing through its anchor entry point --
// the same way the Swordfish tests drive find_swordfish -- to cover both
// orientations and the rejection paths a happy-path solve does not isolate. Since
// there is no predicate to inspect, they read the recorded XWingFinding's fields
// directly (the finding type is visible via analyzer-xwing.h -- no friendship
// needed).

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

    XWingTechnique xw;  // needed for apply() below; find_xwing is static
    FindingList found;
    // Anchor on (0,1), the first 7 of row 0 -- find_xwing's top-left corner.
    check(XWingTechnique::find_xwing(board, cell_at(board, 0, 1), V, found), "row X-Wing detected with anchor (0,1)");
    check(found.size() == 1, "exactly one X-Wing recorded");
    auto const *f = only<XWingFinding>(found);
    check(f, "the recorded finding is an XWingFinding");
    if (f) {
        check(f->is_row_based, "recorded X-Wing is row-based");
        check(f->value == V, "recorded X-Wing is for value 7");
    }

    bool acted = xw.apply(board, found);
    check(acted, "apply reports an elimination");
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

    XWingTechnique xw;  // needed for apply() below; find_xwing is static
    FindingList found;
    check(XWingTechnique::find_xwing(board, cell_at(board, 1, 0), V, found), "column X-Wing detected with anchor (1,0)");
    check(found.size() == 1, "exactly one X-Wing recorded");
    auto const *f = only<XWingFinding>(found);
    check(f, "the recorded finding is an XWingFinding");
    if (f) {
        check(!f->is_row_based, "recorded X-Wing is column-based");
        check(f->value == V, "recorded X-Wing is for value 7");
    }

    bool acted = xw.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 1, 6, V), "stray 7 at (1,6) eliminated from row 1");
    check(!has_candidate(board, 5, 7, V), "stray 7 at (5,7) eliminated from row 5");
    check(has_candidate(board, 1, 0, V) && has_candidate(board, 5, 0, V)
       && has_candidate(board, 1, 3, V) && has_candidate(board, 5, 3, V),
          "all four corner cells kept candidate 7");
}

// A perfect rectangle with nothing to eliminate must not be recorded (recording
// it would assert in apply). Rows 0 and 3 hold 7 in columns 1 and 5 and nowhere
// else, so neither column carries a third candidate.
void test_xwing_no_elimination() {
    std::cout << "[x-wing] a rectangle with nothing to eliminate is not recorded\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5} });

    FindingList found;
    check(!XWingTechnique::find_xwing(board, cell_at(board, 0, 1), V, found),
          "no X-Wing reported when there is nothing to eliminate");
    check(found.empty(), "no X-Wing recorded");
}

// Near-misses: the partner row shares only one of the anchor's two columns, so
// the fourth corner is absent. find_xwing has two containment checks -- one per
// corner of the partner -- and which rejects depends on which column is the odd
// one out, so cover both.
void test_xwing_misaligned_not_found() {
    std::cout << "[x-wing] misaligned candidate lines yield no pattern\n";
    const Value V = kSeven;

    // Partner's SECOND candidate is off the rectangle: row 0 in cols 1,5; row 3
    // in cols 1,8 -- shares col 1, misses col 5 (rejected at the other_eset check).
    {
        Board board = empty_board();
        confine_value(board, V, { {0,1},{0,5}, {3,1},{3,8}, {7,5} });
        FindingList found;
        check(!XWingTechnique::find_xwing(board, cell_at(board, 0, 1), V, found),
              "no X-Wing when the partner's second candidate is off the rectangle");
        check(found.empty(), "nothing recorded");
    }

    // Partner's FIRST candidate is off the rectangle: row 0 in cols 1,5; row 3 in
    // cols 2,5 -- shares col 5, misses col 1 (rejected at the eset check).
    {
        Board board = empty_board();
        confine_value(board, V, { {0,1},{0,5}, {3,2},{3,5} });
        FindingList found;
        check(!XWingTechnique::find_xwing(board, cell_at(board, 0, 1), V, found),
              "no X-Wing when the partner's first candidate is off the rectangle");
        check(found.empty(), "nothing recorded");
    }
}

// find_xwing canonicalises on the first candidate of the anchor's line: anchored
// on a line's *second* candidate it bails at once (a row X-Wing is recorded only
// when anchored on its first candidate, so find() never double-records it).
// Same board as the row-based test, which finds the pattern from (0,1); from
// (0,5) it must find nothing.
void test_xwing_anchor_not_first() {
    std::cout << "[x-wing] anchoring on a non-first candidate finds nothing\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5}, {6,1}, {7,5} });

    FindingList found;
    check(!XWingTechnique::find_xwing(board, cell_at(board, 0, 5), V, found),
          "no X-Wing reported when anchored on the row's second candidate");
    check(found.empty(), "nothing recorded from the non-first anchor");
}

// ===========================================================================
// Finned X-Wing
// ===========================================================================

// A row-based finned X-Wing on value 7. Rows 0 and 3 are the base sets, columns
// 1 and 5 the cover. Row 3 carries one extra 7, the fin at (3,2), inside the
// nonet rows 3-5 / columns 0-2. Only cells in that nonet, on a cover column and
// outside the base rows, are eliminable -- so the stray at (4,1) goes and
// nothing else does. Note the position is invisible to plain X-Wing: the base
// rows span three columns, not two.
//
//        c1 c2 c5
//   r0    7     7      <- clean base row
//   r3    7  7  7      <- base row, plus the fin at c2
//   r4    7            <- cover column, fin's nonet, not a base row: eliminated
void test_finnedxwing_row_based() {
    std::cout << "[finned x-wing] row-based detection, action\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5}, {3,2}, {4,1} });

    FinnedXWingTechnique fx;  // needed for apply() below; find_finned_xwing is static
    FindingList found;
    check(FinnedXWingTechnique::find_finned_xwing(board, cell_at(board, 0, 1), V, found),
          "row finned X-Wing detected with anchor (0,1)");
    check(found.size() == 1, "exactly one finned X-Wing recorded");
    auto const *f = only<FinnedXWingFinding>(found);
    check(f, "the recorded finding is a FinnedXWingFinding");
    if (f) {
        check(f->is_row_based, "recorded pattern is row-based");
        check(f->value == V, "recorded pattern is for value 7");
        check(f->fins.size() == 1 && f->fins[0] == Coord(3, 2),
              "the fin at (3,2) is recorded -- apply() cannot re-derive the cover without it");
    }

    bool acted = fx.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 4, 1, V), "stray 7 at (4,1) eliminated");
    check(has_candidate(board, 3, 2, V), "the fin at (3,2) survives: it sits in a base row");
    check(has_candidate(board, 0, 1, V) && has_candidate(board, 0, 5, V)
       && has_candidate(board, 3, 1, V) && has_candidate(board, 3, 5, V),
          "all four fish corners kept candidate 7");
}

// The transpose: a column-based finned X-Wing on value 7, base columns 0 and 3,
// cover rows 1 and 5, fin at (3,3) in the nonet rows 3-5 / columns 3-5. Column 3
// holds just *one* cover candidate, at (5,3), which makes this a sashimi shape --
// deleting the fin would leave the fish a corner short. It needs no separate code
// path, which is what this case pins.
//
// Anchoring on (1,0) exercises the column orientation because row 1 holds a
// single 7: the row search bails on the two-per-base-line floor before the column
// search runs. (For plain X-Wing the same effect comes from a row with three;
// here extra candidates are the point, so the bail has to come from below.)
//
//        c0 c3 c4
//   r1    7             <- cover row, hit by base column 0 only
//   r3       7          <- the fin
//   r5    7  7  7       <- cover row; (5,4) is eliminated
void test_finnedxwing_column_based() {
    std::cout << "[finned x-wing] column-based (sashimi) detection, action\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {1,0},{5,0}, {5,3}, {3,3}, {5,4} });

    FinnedXWingTechnique fx;  // needed for apply() below; find_finned_xwing is static
    FindingList found;
    check(FinnedXWingTechnique::find_finned_xwing(board, cell_at(board, 1, 0), V, found),
          "column finned X-Wing detected with anchor (1,0)");
    check(found.size() == 1, "exactly one finned X-Wing recorded");
    auto const *f = only<FinnedXWingFinding>(found);
    check(f, "the recorded finding is a FinnedXWingFinding");
    if (f) {
        check(!f->is_row_based, "recorded pattern is column-based");
        check(f->fins.size() == 1 && f->fins[0] == Coord(3, 3), "the fin at (3,3) is recorded");
    }

    bool acted = fx.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 5, 4, V), "stray 7 at (5,4) eliminated");
    check(has_candidate(board, 3, 3, V), "the fin at (3,3) survives: it sits in a base column");
    check(has_candidate(board, 1, 0, V) && has_candidate(board, 5, 0, V) && has_candidate(board, 5, 3, V),
          "the three fish cells kept candidate 7");
}

// The one-nonet rule, isolated. Row 3 gets a second fin at (3,6), in a different
// nonet from (3,2), so no cell sees both fins and no choice of two cover columns
// rescues the position -- every other pair leaves fins straddling nonets too.
// Delete the (3,6) entry and the row-based pattern of the accept case above is
// found again, which is what makes this a test of the one-nonet gate specifically.
//
// (7,1) is scaffolding, and worth explaining rather than leaving to be
// rediscovered. Read down the columns instead of across the rows and this shape
// contains a *second*, sound finned X-Wing: base columns 1 and 5, cover rows 0
// and 3, with the row pattern's eliminable cell (4,1) serving as the fin and the
// row pattern's fin (3,2) as the eliminable cell. The two readings always come in
// pairs like that, so without something to break the column reading this case
// would pass on a found-nothing that had nothing to do with nonets. A second
// candidate at (7,1) puts two fins in two nonets down column 1 as well, which
// rejects every column reading; row 7 holds only that one candidate, so it can
// never serve as a base row itself.
void test_finnedxwing_fins_in_two_nonets_rejected() {
    std::cout << "[finned x-wing] fins spread across two nonets are not a pattern\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5}, {3,2}, {3,6}, {4,1}, {7,1} });

    FindingList found;
    check(!FinnedXWingTechnique::find_finned_xwing(board, cell_at(board, 0, 1), V, found),
          "no finned X-Wing reported when the fins occupy two nonets");
    check(found.empty(), "nothing recorded");
}

// The base-coverage rule, isolated. Row 3's 7s are at (3,0) and (3,2), neither on
// a cover column, so choosing columns 1 and 5 as the cover makes row 3 *all*
// fins. Both fins do share one nonet, and that nonet does meet cover column 1 at
// an eliminable (4,1) -- so without the gate this position records a finding and
// strikes a true candidate. It is not a fish: with every fin false, row 3 would
// hold no 7 at all, which is a contradiction rather than a confinement, and a
// different (stronger) deduction than this technique makes.
//
//        c0 c1 c2 c5
//   r0       7     7    <- the only base row with a cover candidate
//   r3    7     7       <- no cover candidate: all fins
//   r4       7          <- what the missing gate would wrongly eliminate
void test_finnedxwing_uncovered_base_line_rejected() {
    std::cout << "[finned x-wing] a base line with no cover candidate is not a pattern\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,0},{3,2}, {4,1} });

    FindingList found;
    check(!FinnedXWingTechnique::find_finned_xwing(board, cell_at(board, 0, 1), V, found),
          "no finned X-Wing reported when a base line lies entirely outside the cover");
    check(found.empty(), "nothing recorded");
}

// The has-eliminations rule, isolated: test_finnedxwing_row_based's position with
// the stray at (4,1) removed. The pattern is still there and still sound, but its
// eliminable set is empty, and a technique that reports a finding it cannot act on
// would trip apply()'s did_act assert. Nothing else about the position changes.
void test_finnedxwing_no_elimination() {
    std::cout << "[finned x-wing] a sound pattern with nothing to eliminate is not reported\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,5}, {3,1},{3,5}, {3,2} });

    FindingList found;
    check(!FinnedXWingTechnique::find_finned_xwing(board, cell_at(board, 0, 1), V, found),
          "no finned X-Wing reported when the fin's nonet holds nothing to strike");
    check(found.empty(), "nothing recorded");
}

// ===========================================================================
// Finned Swordfish
// ===========================================================================

// A row-based finned Swordfish on value 7. Rows 0, 3 and 4 are the base sets,
// columns 1, 4 and 5 the cover. Row 4 carries one extra 7, the fin at (4,3),
// inside the nonet rows 3-5 / columns 3-5. Only cells in that nonet, on a cover
// column and outside the base rows, are eliminable -- so the stray at (5,5) goes
// and nothing else does. The position is invisible to plain Swordfish: the base
// rows span four columns, not three.
//
//        c1 c3 c4 c5
//   r0    7     7        <- clean base row
//   r3    7        7     <- base row; (3,5) is in the fin's nonet on a cover column
//   r4    7  7     7     <- base row, plus the fin at c3
//   r5             7     <- cover column, fin's nonet, not a base row: eliminated
//
// Two of the three base rows have a cell inside the fin's nonet on a cover column
// -- (3,5) and (4,5) -- and both must survive. That is deliberate, and it is what
// gives this case teeth against the base-line exclusion in the elimination scan:
// drop `cset3`/`b3` from those three-way tests and (4,5) is struck, which is
// unsound rather than merely wrong. Only two can ever be witnessed at once: the
// nonet's band holds three rows, one of which must be a non-base row for anything
// to be eliminable, so the base rows inside it are adjacent in the anchor order.
// (3,5) covers `b2` here and test_finnedswordfish_column_based covers `b1`.
//
// The fin sits at column 3 rather than at a column below the cover so that the
// cover {c1,c4,c5} is the *first* triple the search tries: cross lines are
// collected in first-encounter order, so a fin left of the cover would put its
// line ahead of a cover line and make some other triple first. That matters for
// test isolation, not for the technique -- it keeps this case's outcome
// independent of the has-eliminations gate, which
// test_finnedswordfish_no_elimination owns. With the fin at c2 instead, breaking
// that gate changed *which* pattern this case records, so this case failed and
// aborted the run before the dedicated one was ever reached.
void test_finnedswordfish_row_based() {
    std::cout << "[finned swordfish] row-based detection, action\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,4}, {3,1},{3,5}, {4,1},{4,3},{4,5}, {5,5} });

    FinnedSwordfishTechnique fs;  // needed for apply() below; find_finned_swordfish is static
    FindingList found;
    check(FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 1), V, found),
          "row finned Swordfish detected with anchor (0,1)");
    check(found.size() == 1, "exactly one finned Swordfish recorded");
    auto const *f = only<FinnedSwordfishFinding>(found);
    check(f, "the recorded finding is a FinnedSwordfishFinding");
    if (f) {
        check(f->is_row_based, "recorded pattern is row-based");
        check(f->value == V, "recorded pattern is for value 7");
        check(f->fins.size() == 1 && f->fins[0] == Coord(4, 3),
              "the fin at (4,3) is recorded -- apply() cannot re-derive the cover without it");
    }

    bool acted = fs.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 5, 5, V), "stray 7 at (5,5) eliminated");
    check(has_candidate(board, 4, 3, V), "the fin at (4,3) survives: it sits in a base row");
    check(has_candidate(board, 3, 5, V),
          "(3,5) survives: in the fin's nonet on a cover column, but on the second base row");
    check(has_candidate(board, 4, 5, V),
          "(4,5) survives: same, on the third base row -- the exclusion must cover all three");
    check(has_candidate(board, 0, 1, V) && has_candidate(board, 0, 4, V)
       && has_candidate(board, 3, 1, V) && has_candidate(board, 4, 1, V),
          "every other base candidate kept candidate 7");
}

// The transpose: a column-based finned Swordfish on value 7, base columns 3, 4 and
// 8, cover rows 3, 5 and 8, fin at (4,4) in the nonet rows 3-5 / columns 3-5. The
// sole eliminable cell is (3,5) -- in that nonet, on cover row 3, outside the base
// columns.
//
// Anchoring on (5,3) exercises the column orientation because row 5 holds a single
// 7: the row search bails on the two-per-base-line floor before the column search
// runs. (For plain X-Wing the same effect comes from a row with three; here extra
// candidates are the point, so the bail has to come from below.)
//
// This is the case that witnesses `b1`: (5,3) lies in the fin's nonet on cover row
// 5 and on the *first* base column, so it survives only because the elimination
// scan excludes every base line. (3,4) witnesses `b2` again.
//
//        c3 c4 c5 c8
//   r3       7  7  7     <- cover row; (3,5) is eliminated, (3,4) is a base cell
//   r4       7           <- the fin
//   r5    7              <- cover row; base column cell inside the fin's nonet
//   r8    7  7     7     <- cover row
void test_finnedswordfish_column_based() {
    std::cout << "[finned swordfish] column-based detection, action\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {5,3},{8,3}, {3,4},{4,4},{8,4}, {3,8},{8,8}, {3,5} });

    FinnedSwordfishTechnique fs;  // needed for apply() below; find_finned_swordfish is static
    FindingList found;
    check(FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 5, 3), V, found),
          "column finned Swordfish detected with anchor (5,3)");
    check(found.size() == 1, "exactly one finned Swordfish recorded");
    auto const *f = only<FinnedSwordfishFinding>(found);
    check(f, "the recorded finding is a FinnedSwordfishFinding");
    if (f) {
        check(!f->is_row_based, "recorded pattern is column-based");
        check(f->fins.size() == 1 && f->fins[0] == Coord(4, 4), "the fin at (4,4) is recorded");
    }

    bool acted = fs.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 3, 5, V), "stray 7 at (3,5) eliminated");
    check(has_candidate(board, 4, 4, V), "the fin at (4,4) survives: it sits in a base column");
    check(has_candidate(board, 5, 3, V),
          "(5,3) survives: in the fin's nonet on a cover row, but on the first base column");
    check(has_candidate(board, 3, 4, V),
          "(3,4) survives: same, on the second base column");
}

// The one-nonet rule, isolated. Row 4 gets a second fin at (4,7), in a different
// nonet from (4,3), so the cover that the accept case above settles on is rejected.
// Delete the (4,7) entry and the row-based pattern of that case is found again,
// which is what makes this a test of the one-nonet gate specifically.
//
// The second fin goes to the *right* of the cover, not the left, and that is what
// makes the gate decisive rather than incidental. Fins are recorded in board order,
// so with (4,7) the first fin is still (4,3) and the nonet apply() would use is
// still rows 3-5 / columns 3-5 -- the one holding the eliminable (5,5). Disable the
// one-nonet gate and this position is therefore *accepted*, striking (5,5) on the
// authority of a fin at (4,7) that does not see it. With the second fin at (4,0)
// instead, the recorded nonet became rows 3-5 / columns 0-2, which holds nothing to
// strike, so the position was rejected by the has-eliminations gate whether the
// one-nonet gate ran or not -- and the case passed with that gate broken.
void test_finnedswordfish_fins_in_two_nonets_rejected() {
    std::cout << "[finned swordfish] fins spread across two nonets are not a pattern\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,4}, {3,1},{3,5}, {4,1},{4,3},{4,5},{4,7}, {5,5} });

    FindingList found;
    check(!FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 1), V, found),
          "no finned Swordfish reported when the fins occupy two nonets");
    check(found.empty(), "nothing recorded");
}

// The base-coverage rule, isolated. Row 4's 7s are at (4,0) and (4,2), neither on a
// cover column, so choosing columns 1, 4 and 5 as the cover makes row 4 *all* fins.
// Both fins do share one nonet, and that nonet does meet cover column 1 at an
// eliminable (5,1) -- so without the gate this position records a finding and
// strikes a true candidate. It is not a fish: with every fin false, row 4 would
// hold no 7 at all, which is a contradiction rather than a confinement, and a
// different (stronger) deduction than this technique makes.
//
//        c0 c1 c2 c4 c5
//   r0       7     7        <- base row with cover candidates
//   r3       7        7     <- base row with cover candidates
//   r4    7     7           <- no cover candidate: all fins
//   r5       7              <- what the missing gate would wrongly eliminate
void test_finnedswordfish_uncovered_base_line_rejected() {
    std::cout << "[finned swordfish] a base line with no cover candidate is not a pattern\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,4}, {3,1},{3,5}, {4,0},{4,2}, {5,1} });

    FindingList found;
    check(!FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 1), V, found),
          "no finned Swordfish reported when a base line lies entirely outside the cover");
    check(found.empty(), "nothing recorded");
}

// The has-eliminations rule, isolated: test_finnedswordfish_row_based's position
// with the stray at (5,5) removed. The pattern is still there and still sound, but
// its eliminable set is empty, and a technique that reports a finding it cannot act
// on would trip apply()'s did_act assert. Nothing else about the position changes.
void test_finnedswordfish_no_elimination() {
    std::cout << "[finned swordfish] a sound pattern with nothing to eliminate is not reported\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,4}, {3,1},{3,5}, {4,1},{4,3},{4,5} });

    FindingList found;
    check(!FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 1), V, found),
          "no finned Swordfish reported when the fin's nonet holds nothing to strike");
    check(found.empty(), "nothing recorded");
}

// The has-eliminations rule again, from the other side, and the reason it names
// *every* base line: test_finnedswordfish_column_based's position with the
// eliminable (3,5) removed. What is left in the fin's nonet on a cover row is
// (5,3), on the *first* base column, and (3,4), on the second -- both part of the
// pattern, neither a target. So the position must be rejected.
//
// This is the case that pins the `cset` term specifically. The two accept cases
// cannot: has_eliminations stops at the first cell it likes, so with a genuine
// elimination present a spurious extra one changes no outcome. Here there is no
// genuine one, so dropping `cset` makes find report a pattern that act then cannot
// act on, and apply()'s did_act assert fires.
void test_finnedswordfish_first_base_line_not_a_target() {
    std::cout << "[finned swordfish] a fin-nonet cover cell on a base line is not an elimination\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {5,3},{8,3}, {3,4},{4,4},{8,4}, {3,8},{8,8} });

    FindingList found;
    check(!FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 5, 3), V, found),
          "no finned Swordfish reported when only base-line cells sit in the fin's nonet");
    check(found.empty(), "nothing recorded");
}

// The cover terms in `act`, and the first one in `find`'s has-eliminations scan.
// Both accept cases above put their eliminable cell on the *last* cover line
// recovered, so neither witnesses the earlier terms: a cell lies on exactly one
// cover line, so a case witnesses exactly the index its eliminable cell sits on.
// This position puts eliminable cells on two of them.
//
// Base rows 0, 3 and 4; cover columns 3, 4 and 0; fins at (3,5) and (4,5), which
// share the nonet rows 3-5 / columns 3-5. That nonet meets cover columns 3 and 4,
// at (5,3) and (5,4), and both are struck. `act` recovers the cover by walking the
// base lines' non-fin candidates in order, so row 0's two cells fix `cover[0]` = c3
// and `cover[1]` = c4, and row 3's (3,0) fixes `cover[2]` = c0. Drop either of the
// first two terms and one of the two strikes goes missing.
//
// Note the geometry caps this at two: every cover line that meets the fin's nonet
// must lie in that nonet's three-column band, and the fin itself occupies one of
// those columns without being a cover column, so at most two cover lines can hold
// an eliminable cell. Three cases are therefore the minimum for three terms, and
// the sole-elimination variant below is the third.
void test_finnedswordfish_eliminations_on_two_cover_lines() {
    std::cout << "[finned swordfish] eliminations on the first two recovered cover lines\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,3},{0,4}, {3,0},{3,5}, {4,0},{4,5}, {5,3},{5,4} });

    FinnedSwordfishTechnique fs;
    FindingList found;
    check(FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 3), V, found),
          "finned Swordfish detected with anchor (0,3)");
    auto const *f = only<FinnedSwordfishFinding>(found);
    check(f, "the recorded finding is a FinnedSwordfishFinding");
    if (f) {
        check(f->is_row_based, "recorded pattern is row-based");
        check(f->fins.size() == 2 && f->fins[0] == Coord(3, 5) && f->fins[1] == Coord(4, 5),
              "both fins recorded, in discovery order");
    }

    bool acted = fs.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 5, 3, V), "stray 7 at (5,3) eliminated -- pins cover[0]");
    check(!has_candidate(board, 5, 4, V), "stray 7 at (5,4) eliminated -- pins cover[1]");
    check(has_candidate(board, 3, 5, V) && has_candidate(board, 4, 5, V),
          "both fins survive: they sit in base rows");
    check(has_candidate(board, 0, 3, V) && has_candidate(board, 0, 4, V)
       && has_candidate(board, 3, 0, V) && has_candidate(board, 4, 0, V),
          "every base candidate kept candidate 7");
}

// The fin *order* the header documents, which the case above records but cannot
// discriminate. Fins come out base-line-major: outer loop over base lines, inner
// walk over each line's candidates. For a row-based pattern that coincides with
// row-major board order -- base rows ascend outermost, so the case above would
// assert the same two coordinates in the same sequence under either rule, and a
// walk rewritten to emit board order passes it. Only a column-based position tells
// them apart, because there base columns ascend outermost while rows descend
// within each.
//
// Base columns 0, 3 and 4; cover rows 1, 3 and 8; fins at (5,3) from base column 3
// and (4,4) from base column 4, sharing the nonet rows 3-5 / columns 3-5.
// Base-line-major records (5,3) then (4,4); board order would give (4,4) first, so
// sorting the recorded fins into board order fails this case and nothing else. (The
// case above does catch a straight *reversal*, so the two are not redundant: it
// rejects reorderings in general, this one rejects the specific plausible rewrite.)
// Neither is backed up by a golden -- no README dump block or run.sh expectation
// carries a multi-fin pattern.
//
// Anchoring on (1,0) exercises the column orientation the same way
// test_finnedswordfish_column_based does: row 1 holds a single 7, so the row search
// bails on the two-per-base-line floor before the column search runs. Column 0
// carries no fin of its own, which is what leaves the two fins in the *later* base
// columns and so lets their rows descend against the walk.
//
// The sole eliminable cell is (3,5): in the fin's nonet, on cover row 3, outside all
// three base columns. (3,3) and (3,4) meet the first two conditions but sit on base
// columns, so this case leans on the base-line exclusion as well.
void test_finnedswordfish_fin_order_is_not_board_order() {
    std::cout << "[finned swordfish] a column-based pattern records fins in discovery order, not board order\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {1,0},{3,0},{8,0}, {3,3},{5,3}, {3,4},{4,4}, {3,5} });

    FinnedSwordfishTechnique fs;
    FindingList found;
    check(FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 1, 0), V, found),
          "column finned Swordfish detected with anchor (1,0)");
    auto const *f = only<FinnedSwordfishFinding>(found);
    check(f, "the recorded finding is a FinnedSwordfishFinding");
    if (f) {
        check(!f->is_row_based, "recorded pattern is column-based");
        check(f->fins.size() == 2, "both fins recorded");
        check(f->fins.size() == 2 && f->fins[0] == Coord(5, 3) && f->fins[1] == Coord(4, 4),
              "fins in base-line-major order: (5,3) from column 3 before (4,4) from column 4, "
              "where board order would put (4,4) first");
    }

    bool acted = fs.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 3, 5, V), "stray 7 at (3,5) eliminated");
    check(has_candidate(board, 5, 3, V) && has_candidate(board, 4, 4, V),
          "both fins survive: they sit in base columns");
    check(has_candidate(board, 3, 3, V) && has_candidate(board, 3, 4, V),
          "(3,3) and (3,4) survive: in the fin's nonet on a cover row, but on base columns");
}

// The first cover term inside `find`'s has-eliminations scan, which the case above
// cannot reach. That scan stops at the first cell it likes, so with eliminable cells
// on two cover lines, dropping the first term still leaves the second to satisfy it
// and the pattern is found anyway. Here the position above loses (5,4), so the sole
// eliminable cell is (5,3), on the first cover line the scan tests. Drop that term
// and no cover triple satisfies the scan, so nothing is reported at all.
void test_finnedswordfish_sole_elimination_on_first_cover_line() {
    std::cout << "[finned swordfish] a sole elimination on the first cover line is still found\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,3},{0,4}, {3,0},{3,5}, {4,0},{4,5}, {5,3} });

    FinnedSwordfishTechnique fs;
    FindingList found;
    check(FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 3), V, found),
          "finned Swordfish detected when only the first cover line has anything to strike");
    bool acted = fs.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 5, 3, V), "stray 7 at (5,3) eliminated");
}

// The same, one term along. Dropping the *second* cover term needs its own case for
// the same short-circuit reason: with the sole eliminable cell on the first cover
// line the scan is satisfied before it ever tests the second. So this is the previous
// position with (5,3) removed instead of (5,4), leaving (5,4) -- on cover column 4,
// the second line the scan tests -- as the only cell to strike. The third term is
// already pinned by test_finnedswordfish_row_based, whose sole eliminable (5,5) sits
// on the last cover line tested.
void test_finnedswordfish_sole_elimination_on_second_cover_line() {
    std::cout << "[finned swordfish] a sole elimination on the second cover line is still found\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,3},{0,4}, {3,0},{3,5}, {4,0},{4,5}, {5,4} });

    FinnedSwordfishTechnique fs;
    FindingList found;
    check(FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 3), V, found),
          "finned Swordfish detected when only the second cover line has anything to strike");
    bool acted = fs.apply(board, found);
    check(acted, "apply reports an elimination");
    check(!has_candidate(board, 5, 4, V), "stray 7 at (5,4) eliminated");
}

// The `crosses < 4` early-out, isolated. Rows 0, 3 and 6 hold 7 only in columns 1,
// 4 and 7 -- a fully confined, finless triple, i.e. a *plain* Swordfish, with a
// stray at (1,4) for it to eliminate. Swordfish runs ahead of this technique in the
// cascade and owns this position; a finned Swordfish needs a fourth cross line for
// a fin to live on, and reporting one here would mean claiming a fin that does not
// exist. Drop the early-out and the assert(!fins.empty()) below it fires, which is
// the shape of the failure this case pins.
//
//        c1 c4 c7
//   r0    7  7           <- base row
//   r1       7           <- the plain Swordfish's stray, not a fin: c4 is a cover column
//   r3       7  7        <- base row
//   r6    7     7        <- base row
void test_finnedswordfish_plain_swordfish_not_reported() {
    std::cout << "[finned swordfish] a confined (finless) triple belongs to plain Swordfish\n";
    Board board = empty_board();
    const Value V = kSeven;
    confine_value(board, V, { {0,1},{0,4}, {1,4}, {3,4},{3,7}, {6,1},{6,7} });

    FindingList found;
    check(!FinnedSwordfishTechnique::find_finned_swordfish(board, cell_at(board, 0, 1), V, found),
          "no finned Swordfish reported for a fully confined triple");
    check(found.empty(), "nothing recorded");
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

    ColorChainFinding chain(V);
    chain.cells[Coord(0,0)] = true;    // green
    chain.cells[Coord(0,4)] = true;    // green -- same row as the other green
    chain.cells[Coord(8,8)] = false;   // red

    check(ColorChainTechnique::test_color_chain(board, chain),
          "test_color_chain reports a same-color-in-unit chain as actionable");

    ColorChainTechnique sc;
    FindingList found;
    found.push_back(std::make_shared<ColorChainFinding>(chain));
    bool acted = sc.apply(board, found);
    check(acted, "apply reports an elimination");
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

    ColorChainFinding chain(V);
    chain.cells[Coord(0,0)] = true;    // green
    chain.cells[Coord(4,4)] = false;   // red -- shares no unit with the green

    check(!ColorChainTechnique::test_color_chain(board, chain),
          "test_color_chain reports a benign chain as not actionable");
}

// ===========================================================================
// Rebinding ctor (issue #7)
// ===========================================================================
//
// Every technique's findings live in one carried member, mFindings,
// initialized by a single hand-written line in the rebinding ctor
// (`mFindings(other.mFindings)`). Drop that line and *every* technique
// silently stops carrying forward. This test isolates exactly that line: it
// builds an analyzer with a known finding, then constructs a second analyzer
// through the rebinding ctor ONLY (never calling analyze() on it), so the
// copied member is the sole possible source of
// its findings. Removing the initializer turns this into a named failure rather
// than a mysterious integration hang.
void test_rebinding_ctor_carries_findings() {
    std::cout << "[rebinding ctor] carries mFindings forward across the state copy (issue #7)\n";

    // A single note cell pinned to one candidate is a naked single; every other
    // cell still carries all nine, so analyze() records exactly one finding.
    Board board = empty_board();
    set_candidates(board, 0, 0, {5});

    Analyzer a(board);
    a.analyze();
    check(AnalyzerTest::findings_bucket_count(a) == 12, "twelve registry buckets (NS, HS, NP, LC, HP, XW, SC, YW, SF, FX, FS, XY)");
    check(AnalyzerTest::findings_total(a) == 1, "the naked single was recorded in a's bucket (HS/NP/LC/HP/XW/SC/YW/SF/FX/FS/XY short-circuited)");

    // Copy the candidate grid and rebind onto it -- this is the ONLY operation
    // under test. b.analyze() is deliberately never called.
    Board board2(board);
    Analyzer b(board2, a);
    check(AnalyzerTest::findings_bucket_count(b) == AnalyzerTest::findings_bucket_count(a),
          "rebinding ctor preserves the bucket count");
    check(AnalyzerTest::findings_total(b) == AnalyzerTest::findings_total(a),
          "rebinding ctor carries the findings forward (drop mFindings(other.mFindings) => this fails)");
    check(AnalyzerTest::findings_render(b) == AnalyzerTest::findings_render(a),
          "the carried findings are byte-identical to the source's");
}

} // namespace

int main() {
    test_swordfish_column_based();
    test_swordfish_row_based();
    test_swordfish_no_elimination();
    test_xychain_detect_and_act();
    test_xychain_best_selection();
    test_ywing_detect_and_act();
    test_ywing_rejects_non_patterns();
    test_notes_set_ops();
    test_naked_pair_accept_and_reject();
    test_hidden_pair_accept_and_reject();
    test_xwing_row_based();
    test_xwing_column_based();
    test_xwing_no_elimination();
    test_xwing_misaligned_not_found();
    test_xwing_anchor_not_first();
    test_finnedxwing_row_based();
    test_finnedxwing_column_based();
    test_finnedxwing_fins_in_two_nonets_rejected();
    test_finnedxwing_uncovered_base_line_rejected();
    test_finnedxwing_no_elimination();
    test_finnedswordfish_row_based();
    test_finnedswordfish_column_based();
    test_finnedswordfish_fins_in_two_nonets_rejected();
    test_finnedswordfish_uncovered_base_line_rejected();
    test_finnedswordfish_no_elimination();
    test_finnedswordfish_first_base_line_not_a_target();
    test_finnedswordfish_eliminations_on_two_cover_lines();
    test_finnedswordfish_fin_order_is_not_board_order();
    test_finnedswordfish_sole_elimination_on_first_cover_line();
    test_finnedswordfish_sole_elimination_on_second_cover_line();
    test_finnedswordfish_plain_swordfish_not_reported();
    test_colorchain_rule2_contradiction();
    test_colorchain_benign_not_actionable();
    test_rebinding_ctor_carries_findings();

    std::cout << "----------------------------------------\n";
    if (failures == 0) { std::cout << "unit: all checks passed\n"; return 0; }
    std::cout << "unit: " << failures << " check(s) failed\n";
    return 1;
}
