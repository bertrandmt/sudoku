// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-xychain.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

namespace {

// The chain under construction, as the search holds it: the cells themselves,
// not their coordinates. Recovering a Cell from a Coord takes Board::at, which
// is reserved to Board's friends and a technique is not one; rather than reach
// for that lookup through some other door, the search keeps what it already had
// in hand and pays the Cell -> Coord conversion once, at the point a chain is
// actually recorded.
//
// Only element 0 points at a cell the board owns (the anchor find_xychain was
// handed). Every later element points into a `std::unordered_set<Cell>` of
// selected candidates, so it is a *snapshot* of that cell taken when
// select_chain_candidates copied it in, not a live board cell. That is
// equivalent here only because find is a pure query: nothing mutates the board
// between the copy and the read.
//
// Lifetime: each frame's candidate set outlives the calls that frame makes, and
// every push_back in extend_chain is popped before that frame exits, so a pointer
// is only ever dereferenced while the set holding it is alive (see extend_chain's
// loop). The one push that is never popped is find_xychain's anchor, element 0,
// which needs no such guarantee: it points into the board, not into a frame.
using ChainCells = std::vector<const Cell *>;

std::vector<Coord> coords_of(const ChainCells &chain) {
    std::vector<Coord> coords;
    coords.reserve(chain.size());
    for (const auto *cell : chain) coords.push_back(cell->coord());
    return coords;
}

bool on_chain(const ChainCells &chain, const Coord &coord) {
    return std::any_of(chain.begin(), chain.end(),
                       [&coord](const Cell *c) { return c->coord() == coord; });
}

// Score `chain`: validate that it links up and that the far end's candidate is
// also a candidate for the initial cell, then collect the off-chain cells that
// would lose `value`. An empty set means "not actionable" -- either the chain does
// not close or it eliminates nothing, a distinction the caller does not need.
// File-local because no whitebox case calls it (see analyzer-xychain.h).
//
// Collecting the coords rather than counting them lets the finding carry its own
// effect, so apply() replays what this saw instead of rediscovering it.
std::set<Coord> test_xychain(const Board &board, const Value &value, const ChainCells &chain) {
    // validate that the chain chains properly, and that the end candidate value is also a
    // candidate for the initial cell.
    Value other_value = value;
    for (const auto *cell : chain) {
        Value this_value = other_value;

        assert(cell->isNote());
        assert(cell->notes().count() == 2);
        if (!cell->check(this_value)) return {};
        other_value = cell->other_value(this_value);
    }
    // is the last "other_value" is the incoming candidate value
    if (other_value != value) return {};

    // yes! collect eliminations
    std::set<Coord> eliminations;
    for (const auto &cell : board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but is value a candidate for it?
        if (!cell.check(value)) continue;

        // yes! but is it on the chain?
        if (on_chain(chain, cell.coord())) continue;

        // no! but can it see both ends?
        if (!board.see_each_other(cell.coord(), chain.front()->coord())) continue;
        if (!board.see_each_other(cell.coord(), chain.back()->coord())) continue;

        // yes! collect it
        eliminations.insert(cell.coord());
    }

    return eliminations;
}

template<class Set>
void select_chain_candidates(const Cell &current, Value value, const Set &set, const std::unordered_set<Coord> &visited, std::unordered_set<Cell> &chain_candidates) {
    assert(set.contains(current));

    for (auto const &cell : set) {
        // is this not the current cell
        if (cell == current) continue;

        // yes! but has it been visited before?
        if (visited.count(cell.coord())) continue;

        // yes! but is it a note?
        if (!cell.isNote()) continue;

        // yes! but does it have only two candidates?
        if (cell.notes().count() != 2) continue;

        // yes! but does it have value as candidate?
        if (!cell.check(value)) continue;

        // yes! ok, this is a bona fide chain candidate; record it
        // unordered_set automatically handles duplicates
        chain_candidates.insert(cell);
    }
}

// Extend `chain` by one cell in every direction it can go, and recurse, stopping
// the moment an actionable chain of exactly `max_len` cells is met.
// `incoming_link_value` is the candidate the previous link resolved into `cell`,
// so the chain continues on `cell`'s *other* candidate. Returns whether anything
// was recorded.
//
// Only chains of exactly `max_len` are tested, not every chain met on the way
// down. Shorter ones were already tested -- and found not actionable, or find()
// would have stopped -- on an earlier sweep, so re-testing them would repeat an
// O(cells) scan per node for a result already known.
//
// A plain recursive function, not a lambda: the std::function was there only so
// the lambda could name itself for the recursive call, and cost a heap allocation
// per search root plus an indirect call at every node (#30). The better reason is
// scoping -- the lambda's own `cell` parameter shadowed find_xychain's anchor cell
// under a capture-all, so an accidental read of the anchor from inside the
// recursion would have compiled silently. Here the anchor is not in scope at all.
//
// The two accumulators find_xychain owns -- the chain under construction and the
// visited set -- are threaded by reference rather than captured.
bool extend_chain(const Board &board, const Cell &cell, Value incoming_link_value,
                  ChainCells &chain, std::unordered_set<Coord> &visited, FindingList &out,
                  size_t max_len) {
    assert(cell.isNote());
    assert(cell.check(incoming_link_value));
    assert(cell.notes().count() == 2);
    assert(out.empty());

    if (chain.size() >= max_len) return false;

    // select cells that can see current cell and share its "other" value
    Value common_link_value = cell.other_value(incoming_link_value);
    std::unordered_set<Cell> candidates;
    select_chain_candidates(cell, common_link_value, board.row(cell), visited, candidates);
    select_chain_candidates(cell, common_link_value, board.column(cell), visited, candidates);
    select_chain_candidates(cell, common_link_value, board.nonet(cell), visited, candidates);

    // Walk the candidates coord-sorted, not in the set's own order. `candidates`
    // is an unordered_set, so its own order is unspecified and differs between
    // standard libraries, and this technique stops at the first actionable chain it
    // meets: among the equal-length chains this frame could reach, which one gets
    // recorded IS this order. It is a result, not a presentation detail.
    //
    // Chain length no longer rides on it -- find() sweeps lengths from short to
    // long, so a chain recorded here is the shortest actionable one on the board
    // whatever order this frame walks in. What is left to this sort is the choice
    // among chains of that same shortest length, which is the whole of what #53
    // asked to have pinned.
    //
    // test_xychain_visit_order pins it (tests/unit/test_analyzer.cpp): a crafted
    // board offering several equal-length continuations, asserting the coord-least
    // is the one recorded. What it pins is the *direction* of this ordering, and it
    // pins that on every toolchain -- reversing the comparator sorts descending, and
    // since a standard library varies the candidate set's iteration order but not its
    // contents, and coords within it are unique, the reversed result is identical
    // everywhere.
    //
    // What no test can pin is that a sort happens at all. Delete it and the order
    // becomes unspecified rather than wrong, so the case may pass by coincidence. It
    // does fail on libc++, where this board's candidates leave the bucket in a
    // non-coord order, but that is one hash table's behaviour and not something to
    // rest on. Deleting it changes no corpus board on this toolchain either, so the
    // black-box suite cannot see it. That asymmetry is inherent, and it is what #53
    // closes against rather than something left undone.
    //
    // `candidates` outlives every recursive call made from this frame, so the
    // pointers pushed below stay valid for as long as they are on the chain,
    // and `ordered` only holds pointers into it.
    std::vector<const Cell *> ordered;
    ordered.reserve(candidates.size());
    for (const Cell &c : candidates) ordered.push_back(&c);
    std::sort(ordered.begin(), ordered.end(),
              [](const Cell *a, const Cell *b) { return a->coord() < b->coord(); });

    for (const auto *next_cell_ptr : ordered) {
        const Cell &next_cell = *next_cell_ptr;
        // proactively extend the chain with next_cell
        chain.push_back(&next_cell);
        visited.insert(next_cell.coord());

        Value next_link_value = next_cell.other_value(common_link_value);
        bool done = false;
        if (chain.size() == max_len) {
            // at the length under test: is the chain valid, and would acting on it
            // have an impact?
            std::set<Coord> eliminations = test_xychain(board, next_link_value, chain);
            if (!eliminations.empty()) {
                auto finding = std::make_shared<const XYChainFinding>(
                    next_link_value, coords_of(chain), std::move(eliminations));
                if (sVerbose) { std::cout << "  [fXY] "; finding->print(std::cout); std::cout << std::endl; }
                out.push_back(finding);
                done = true;
            }
        } else {
            // not yet: go deeper
            done = extend_chain(board, next_cell, common_link_value, chain, visited, out, max_len);
        }

        // backtrack
        chain.pop_back();
        visited.erase(next_cell.coord());

        // stop at the first actionable chain: acting on it moves the state forward,
        // and SolverState::act re-runs analyze(), so anything else this frame could
        // reach is re-derived against the board as it will then be.
        if (done) return true;
    }

    return false;
}

// Clear `entry.value` from every cell of the effect `entry` recorded.
//
// The set is replayed, not rediscovered. Rescanning one chain end's units and
// re-testing `see_each_other` against the other -- which is what this used to do --
// re-derives from scratch the answer find() already had in hand, and made the
// emission order depend on which end happened to be front(). Replaying takes the
// set's own Coord order instead.
//
// Every cell in the set still carries the value: find() is a pure query, nothing
// mutates the board between it and here, and there is only ever one finding. Hence
// the assert rather than a skip -- a failure here means the finding and the board
// disagree, which no legitimate path produces.
bool act_on_xychain(Board &board, const XYChainFinding &entry) {
    bool did_act = false;

    for (const auto &coord : entry.eliminations) {
        std::cout << "[XY] " << coord << " x" << entry.value
                  << " ({" << entry.chain.front() << ":..:" << entry.chain.back() << "}#" << entry.value << ")" << std::endl;
        bool cleared = board.clear_note_at(coord, entry.value);
        assert(cleared);
        did_act |= cleared;
    }

    return did_act;
}

} // namespace

void XYChainFinding::print(std::ostream &outs) const {
    outs << "{";
    for (size_t i = 0; i < chain.size(); i++) {
        if (i > 0) outs << ":";
        outs << chain[i];
    }
    outs << "}#" << value
         << "x" << eliminations.size();
}

bool XYChainTechnique::find_xychain(const Board &board, const Cell &cell, const Value &value, size_t max_len, FindingList &out) {
    assert(cell.isNote());
    assert(cell.notes().count() == 2);
    assert(cell.check(value));

    if (!out.empty()) return false;   // already found; find()'s sweep is over

    ChainCells chain;
    std::unordered_set<Coord> visited;

    chain.push_back(&cell);
    visited.insert(cell.coord());

    return extend_chain(board, cell, value, chain, visited, out, max_len);
}

// https://www.sudokuwiki.org/XY_Chains
// An XY-Chain is a sequence of XY-cells (cells with exactly 2 candidates)
// where each adjacent pair shares exactly one candidate.
// If the chain starts and ends with the same candidate, that candidate
// can be eliminated from cells that can see both chain ends.
//
// For this heuristic we look for the SHORTEST actionable chain on the board and
// act on that one, which makes XY-chain a first-hit technique like the fish and
// simple coloring: acting on one finding moves the state forward, and
// SolverState::act re-runs analyze(), so a second chain is found against the board
// as it will then be rather than as it is now.
//
// The sweep is iterative-deepening: try every chain of length 2, then every chain
// of length 3, and stop at the first length that yields something actionable. The
// length loop is OUTSIDE the anchor loop on purpose -- inside it, "shortest" would
// mean shortest from whichever anchor came first, which is not the same thing and
// is not a property of the board.
//
// Why length-ordered rather than exhaustive-then-ranked (which is what this did
// before, and what issue #36 first proposed replacing it with -- see the reversal
// comment there): on a reduced grid nearly every unsolved cell is bi-value, so the
// chains form one dense web and enumerating it yields dozens of long chains whose
// eliminations are all sound and none of which a person would trace. Searching
// short-first finds a *better* chain, not merely fewer: on the issue's worked
// board it acts on 5 cells where exhaustive ranking reached for 17.
//
// Nothing is forbidden by this. A long chain is still found -- just not while a
// shorter one exists. That is the difference between ordering the search and
// capping it: a cap would lose the boards whose only chain is long.
bool XYChainTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    // A chain visits distinct bi-value cells, so it cannot be longer than there are
    // of them. That bound is what the sweep costs when the board has no actionable
    // chain at all, which is the common case, so it is worth counting rather than
    // running to 81.
    size_t bivalue = 0;
    for (const auto &cell : board.cells())
        if (cell.isNote() && cell.notes().count() == 2) bivalue++;

    for (size_t max_len = 2; max_len <= bivalue && out.empty(); max_len++) {
        for (const auto &cell : board.cells()) {
            // is this a note cell?
            if (!cell.isNote()) continue;

            // yes! but does it have only two candidates?
            if (cell.notes().count() != 2) continue;

            // yes! attempt to build chains from this cell for each candidate value
            auto values = cell.notes().values();
            if (find_xychain(board, cell, values[0], max_len, out)) break;
            if (find_xychain(board, cell, values[1], max_len, out)) break;
        }
    }

    assert(out.size() <= 1);
    return !out.empty();
}

bool XYChainTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    bool did_act = act_on_xychain(board, bucket_cast<XYChainFinding>(*mine.front()));

    // the finding refers into the bucket; it must not be touched past this point.
    mine.clear();

    assert(did_act);
    return did_act;
}
