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
// would lose `value`. An empty set means "not actionable" -- either the chain
// does not close or it eliminates nothing, a distinction the caller does not
// need. File-local because no whitebox case calls it (see analyzer-xychain.h).
//
// Collecting the coords rather than counting them is what lets a finding carry
// its own effect: the retention rule compares effects, and apply() replays this
// set instead of rediscovering it against a board that other findings have
// meanwhile changed.
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

// Extend `chain` by one cell in every direction it can go, offering each
// actionable chain met along the way to record_if_maximal, and recurse.
// `incoming_link_value` is the candidate the previous link resolved into `cell`,
// so the chain continues on `cell`'s *other* candidate. Returns whether any
// offer was accepted.
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

    // `candidates` is an unordered_set and this walks it in its own order, which
    // is unspecified and differs between standard libraries. That is deliberate,
    // and it is safe here only because nothing downstream of this loop depends on
    // the order offers arrive in:
    //
    //  - which chains are retained is order-independent by construction
    //    (record_if_maximal), and
    //  - the order they are emitted in is fixed by find()'s canonical sort, not
    //    by the order they were found.
    //
    // A coord-sort used to stand here for exactly the reasons those two bullets
    // now cover: the recorded chain reached stdout in traversal order, and
    // trim-to-one let whichever equally-desirable chain arrived first win. It is
    // gone because it no longer protects anything -- verified by replaying the
    // whole board corpus with the sort restored and diffing the transcripts, which
    // are byte-identical. Restoring it as insurance would re-introduce a mechanism
    // that no test can fail, which is the state #53 filed.
    //
    // `candidates` outlives every recursive call made from this frame, so the
    // pointers pushed below stay valid for as long as they are on the chain.
    for (const Cell &next_cell : candidates) {
        // proactively extend the chain with next_cell
        chain.push_back(&next_cell);
        visited.insert(next_cell.coord());

        // is the chain valid and would acting on it have an impact
        Value next_link_value = next_cell.other_value(common_link_value);
        std::set<Coord> eliminations = test_xychain(board, next_link_value, chain);
        if (!eliminations.empty()) {
            // yes! offer it; retained unless an already-recorded effect covers it
            did_find |= XYChainTechnique::record_if_maximal(
                out, XYChainFinding{next_link_value, coords_of(chain), std::move(eliminations)});
        }

        // recurse
        did_find |= extend_chain(board, next_cell, common_link_value, chain, visited, out);

        // backtrack
        chain.pop_back();
        visited.erase(next_cell.coord());
    }

    return did_find;
}

// Clear `entry.value` from every cell of the effect `entry` recorded.
//
// The set is replayed rather than rediscovered. Rediscovering it -- rescanning
// one chain end's units and re-testing `see_each_other` against the other, which
// is what this used to do -- reads a board that earlier findings in the same
// apply() have already changed, and re-derives from scratch the answer find()
// had in hand. Replaying also means the emission order is the set's own (Coord
// order) rather than a scan order that depended on which chain end came first.
//
// clear_note_at is the guard for the one way two retained effects can interact:
// non-nesting effects may still overlap, so a cell a previous chain already
// cleared reports false here and is skipped, printing nothing. That is why the
// shared elimination shows up once and not once per justification.
bool act_on_xychain(Board &board, const XYChainFinding &entry) {
    bool did_act = false;

    for (const auto &coord : entry.eliminations) {
        // has an earlier finding in this same apply() already cleared it?
        if (!board.clear_note_at(coord, entry.value)) continue;

        // no! ELIMINATE!
        std::cout << "[XY] " << coord << " x" << entry.value
                  << " ({" << entry.chain.front() << ":..:" << entry.chain.back() << "}#" << entry.value << ")" << std::endl;
        did_act = true;
    }

    return did_act;
}

} // namespace

bool XYChainFinding::subsumes(const XYChainFinding &other) const {
    if (value != other.value) return false;
    return std::includes(eliminations.begin(), eliminations.end(),
                         other.eliminations.begin(), other.eliminations.end());
}

bool XYChainFinding::tighter_than(const XYChainFinding &other) const {
    assert(value == other.value && eliminations == other.eliminations);
    if (chain.size() != other.chain.size()) return chain.size() < other.chain.size();
    return chain < other.chain;
}

bool XYChainFinding::precedes(const XYChainFinding &other) const {
    if (value != other.value) return value < other.value;
    if (eliminations != other.eliminations) return eliminations < other.eliminations;
    return chain < other.chain;
}

void XYChainFinding::print(std::ostream &outs) const {
    outs << "{";
    for (size_t i = 0; i < chain.size(); i++) {
        if (i > 0) outs << ":";
        outs << chain[i];
    }
    outs << "}#" << value
         << "x" << eliminations.size();
}

bool XYChainTechnique::record_if_maximal(FindingList &out, const XYChainFinding &candidate) {
    // Does some retained effect already cover this one? Then this finding adds
    // nothing to act on -- with one exception: an entry with an *identical* effect
    // covers the candidate and is covered by it, so that pair would reject on
    // either arrival order and the tie has to be settled on the chains instead.
    for (auto const &recorded : out) {
        auto const &entry = bucket_cast<XYChainFinding>(*recorded);
        if (!entry.subsumes(candidate)) continue;
        if (entry.eliminations != candidate.eliminations) return false;
        if (!candidate.tighter_than(entry)) return false;
    }

    // No. Copy the candidate before disturbing `out`: it may itself be a
    // reference into the bucket, and an entry dropped below could be the last
    // owner of it.
    auto finding = std::make_shared<const XYChainFinding>(candidate);

    // Drop everything the newcomer covers -- both the strictly narrower effects
    // and the looser chain for an identical one. `out` therefore never holds a
    // finding covered by another, whatever order the search offered them in.
    std::erase_if(out, [&finding](const auto &recorded) {
        return finding->subsumes(bucket_cast<XYChainFinding>(*recorded));
    });

    // Insert at the canonical position rather than appending. Keeping `out` sorted
    // here, instead of sorting it once the search is done, is what makes the
    // emission order part of this function's tested contract: a separate sort at
    // the end of find() is a single statement that no whitebox case can reach, and
    // deleting it changes the output of boards the black-box suite does not
    // compare line-for-line -- the same shape of unpinned output dependency #53
    // filed against the traversal sort this replaces. Erasing preserves sort
    // order and so does inserting here, so the invariant holds inductively.
    auto at = std::lower_bound(out.begin(), out.end(), finding,
                               [](const auto &a, const auto &b) {
                                   return bucket_cast<XYChainFinding>(*a)
                                       .precedes(bucket_cast<XYChainFinding>(*b));
                               });
    out.insert(at, finding);
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
// For this heuristic, we will find all chains and act on every distinct
// elimination effect among them, keeping the inclusion-maximal ones. Each
// elimination is a sound deduction from this one board snapshot -- candidate `v`
// provably cannot occupy a cell seeing both ends of its chain -- and eliminations
// only remove possibilities, so a set of them derived from one snapshot is jointly
// valid and can be applied together. Acting on one and discarding the rest, which
// is what this used to do, threw away sound deductions and re-derived them on
// later passes (#36).
//
// (Another chain's elimination can break a *different* chain's structural
// precondition, its interior cells being bi-value. That invalidates the structure,
// not the conclusion already drawn from it, which stays true. `see_each_other` is
// purely geometric and is unaffected.)
bool XYChainTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    for (const auto &cell : board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have only two candidates?
        if (cell.notes().count() != 2) continue;

        // yes! attempt to build chains from this cell for each candidate value
        auto values = cell.notes().values();
        find_xychain(board, cell, values[0], out);
        find_xychain(board, cell, values[1], out);
    }

    // `out` is already in canonical order: record_if_maximal keeps it that way as
    // it goes, which is the only thing deciding what order the findings are emitted
    // and applied in. The search itself walks unordered containers and is free to
    // discover them in any order (see extend_chain).

    // The [fXY] trace is emitted here, once the list has settled, rather than at
    // each offer the way every other technique emits at each recording. Offers are
    // not findings here: an offer can be dropped later by a broader one, so tracing
    // at offer time would report chains the technique does not go on to act on, and
    // would put the search's discovery order back into the output.
    if (sVerbose) {
        for (auto const &finding : out) {
            std::cout << "  [fXY] "; finding->print(std::cout); std::cout << std::endl;
        }
    }

    return !out.empty();
}

bool XYChainTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;

    // Accumulated, not assigned: a finding whose cells are all covered by the
    // *union* of earlier ones clears nothing, and the assert below asks whether
    // anything happened at all. The first finding always acts -- find() saw its
    // effect on this same board -- so the assert holds, but only because every
    // result is folded in.
    for (auto const &finding : mine) {
        did_act |= act_on_xychain(board, bucket_cast<XYChainFinding>(*finding));
    }

    // The findings refer into the bucket; none must be touched past this point.
    mine.clear();

    assert(did_act);
    return did_act;
}
