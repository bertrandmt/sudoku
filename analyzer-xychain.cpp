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
// every push_back is popped before the frame exits, so a pointer is only ever
// dereferenced while the set holding it is alive (see extend_chain's loop).
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
// also a candidate for the initial cell, then count the off-chain cells that
// would lose `value`. Zero means "not actionable" -- either the chain does not
// close or it eliminates nothing, a distinction the caller does not need.
// File-local because no whitebox case calls it (see analyzer-xychain.h).
size_t test_xychain(const Board &board, const Value &value, const ChainCells &chain) {
    // validate that the chain chains properly, and that the end candidate value is also a
    // candidate for the initial cell.
    Value other_value = value;
    for (const auto *cell : chain) {
        Value this_value = other_value;

        assert(cell->isNote());
        assert(cell->notes().count() == 2);
        if (!cell->check(this_value)) return 0;
        other_value = cell->other_value(this_value);
    }
    // is the last "other_value" is the incoming candidate value
    if (other_value != value) return 0;

    // yes! count eliminations
    size_t num_elim = 0;
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

        // yes! count it
        num_elim++;
    }

    return num_elim;
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

// Extend `chain` by one cell in every direction it can go, offering each
// actionable chain met along the way to record_if_best, and recurse.
// `incoming_link_value` is the candidate the previous link resolved into `cell`,
// so the chain continues on `cell`'s *other* candidate. Returns whether any
// offer was accepted.
//
// A plain recursive function, not a lambda: recursing through a std::function
// cost a heap allocation and an indirect call (#30). The two accumulators
// find_xychain owns -- the chain under construction and the visited set -- are
// threaded by reference rather than captured, which also makes it visible at the
// call site that the recursion mutates them.
bool extend_chain(const Board &board, const Cell &cell, Value incoming_link_value,
                  ChainCells &chain, std::unordered_set<Coord> &visited, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(incoming_link_value));
    assert(cell.notes().count() == 2);

    bool did_find = false;

    // select cells that can see current cell and share its "other" value
    Value common_link_value = cell.other_value(incoming_link_value);
    std::unordered_set<Cell> candidates;
    select_chain_candidates(cell, common_link_value, board.row(cell), visited, candidates);
    select_chain_candidates(cell, common_link_value, board.column(cell), visited, candidates);
    select_chain_candidates(cell, common_link_value, board.nonet(cell), visited, candidates);

    // Walk the candidates coord-sorted, not in the set's own order. Two
    // reasons, and the second is the load-bearing one:
    //
    //  - `candidates` is an unordered_set, so its iteration order is
    //    unspecified and differs between standard libraries. It reaches
    //    stdout: the chain is recorded in traversal order, so it fixes both
    //    the "{c1:c2:..}" dump and the order of apply()'s [XY] lines.
    //  - record_if_best keeps ONE chain and rejects ties, so whichever
    //    equally-desirable chain is *offered first* wins. Discovery order is
    //    therefore part of the result, not just of the presentation.
    //
    // Measured over the 34-board corpus (31 in notes.txt plus run.sh's 9
    // fixtures, 6 of which are the same boards), sorting changes the output of
    // exactly one, and there it selects the same
    // chain traversed in the opposite direction: same endpoints, same value,
    // same two eliminations, same final grid. So this buys determinism without
    // changing what the solver concludes on any board we have. It is not a
    // proof for all boards -- two genuinely different chains tied on
    // (elimination count, length) would still be resolved by whoever is
    // offered first -- which is one more reason to prefer issue #36's
    // greedy-on-all-distinct-effects rewrite over this trim-to-one.
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

        // is the chain valid and would acting on it have an impact
        Value next_link_value = next_cell.other_value(common_link_value);
        size_t num_elim = test_xychain(board, next_link_value, chain);
        if (num_elim > 0) {
            // yes! offer it; only a strictly more desirable chain is kept
            did_find |= XYChainTechnique::record_if_best(out, XYChainFinding{next_link_value, coords_of(chain), num_elim});
        }

        // recurse
        did_find |= extend_chain(board, next_cell, common_link_value, chain, visited, out);

        // backtrack
        chain.pop_back();
        visited.erase(next_cell.coord());
    }

    return did_find;
}

// Clear `entry.value` from every cell of `chain_front_set` that sees the far end
// of the chain and is not on the chain itself.
template<class Set>
bool act_on_xychain(Board &board, const XYChainFinding &entry, const Set &chain_front_set) {
    bool did_act = false;

    // For each cell that can see the other end of the chain, eliminate the chain value
    for (const auto &cell : chain_front_set) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but is it a candidate for the entry's value?
        if (!cell.check(entry.value)) continue;

        // yes! but is it on the chain?
        if (std::find(entry.chain.begin(), entry.chain.end(), cell.coord()) != entry.chain.end()) continue;

        // no! but can it see the other end of the chain?
        if (!board.see_each_other(cell.coord(), entry.chain.back())) continue;

        // yes! ELIMINATE!
        std::cout << "[XY] " << cell.coord() << " x" << entry.value
                  << " ({" << entry.chain.front() << ":..:" << entry.chain.back() << "}#" << entry.value << ")" << std::endl;
        board.clear_note_at(cell.coord(), entry.value);
        did_act = true;
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
         << "x" << num_elim;
}

bool XYChainTechnique::record_if_best(FindingList &out, const XYChainFinding &candidate) {
    // `out` holds at most one chain: the most desirable offered so far.
    if (!out.empty()) {
        assert(out.size() == 1);
        auto const &best = bucket_cast<XYChainFinding>(*out.front());

        // is the same chain already recorded (same value, same endpoints)?
        // then keep the one found first.
        if (best == candidate) return false;

        // no! but is the candidate strictly more desirable? `!(candidate < best)`
        // also rejects the equally-desirable case (same elimination count, same
        // length): a tie leaves the incumbent in place.
        if (!(candidate < best)) return false;
    }

    // Copy `candidate` before dropping the incumbent, and clear only on the path
    // that immediately refills (clearing an empty vector is a no-op, so the
    // first-recording path pays nothing). Were `candidate` ever to alias the
    // incumbent, clearing first would drop the last reference to it and leave
    // the copy below reading a destroyed object. No caller can reach that today
    // -- an alias trips the `best == candidate` gate above, operator== being
    // reflexive -- but that makes this function's safety rest on a property of a
    // gate two lines up, which is not where a reader would look for it.
    auto finding = std::make_shared<const XYChainFinding>(candidate);
    if (sVerbose) { std::cout << "  [fXY] "; finding->print(std::cout); std::cout << std::endl; }
    out.clear();
    out.push_back(finding);
    return true;
}

bool XYChainTechnique::find_xychain(const Board &board, const Cell &cell, const Value &value, FindingList &out) {
    assert(cell.isNote());
    assert(cell.notes().count() == 2);
    assert(cell.check(value));

    ChainCells chain;
    std::unordered_set<Coord> visited;

    chain.push_back(&cell);
    visited.insert(cell.coord());

    return extend_chain(board, cell, value, chain, visited, out);
}

// https://www.sudokuwiki.org/XY_Chains
// An XY-Chain is a sequence of XY-cells (cells with exactly 2 candidates)
// where each adjacent pair shares exactly one candidate.
// If the chain starts and ends with the same candidate, that candidate
// can be eliminated from cells that can see both chain ends.
//
// For this heuristic, we will find all chains, rank them by number of
// eliminations (greater is better) and length (shorter is better), and act
// only on the most desirable chain.
bool XYChainTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (const auto &cell : board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have only two candidates?
        if (cell.notes().count() != 2) continue;

        // yes! attempt to build chains from this cell for each candidate value
        auto values = cell.notes().values();
        did_find |= find_xychain(board, cell, values[0], out);
        did_find |= find_xychain(board, cell, values[1], out);
    }

    return did_find;
}

bool XYChainTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    auto const &entry = bucket_cast<XYChainFinding>(*mine.front());

    bool did_act = false;

    did_act |= act_on_xychain(board, entry, board.row(entry.chain.front()));
    did_act |= act_on_xychain(board, entry, board.column(entry.chain.front()));
    did_act |= act_on_xychain(board, entry, board.nonet(entry.chain.front()));

    // `entry` refers into the bucket; it must not be touched past this point.
    mine.clear();

    assert(did_act);
    return did_act;
}
