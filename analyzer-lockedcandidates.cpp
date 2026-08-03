// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-lockedcandidates.h"
#include "board.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

namespace {
// File-local: LC has no whitebox hooks, so nothing outside this TU needs to name
// or downcast the finding. print() format: "{c1,c2,...}#value[^unit]".
struct LockedCandidatesFinding : Finding {
    std::vector<Coord> coords;
    Value value;
    Unit  unit;
    LockedCandidatesFinding(std::vector<Coord> c, Value v, Unit u)
        : coords(std::move(c)), value(v), unit(u) { }

    // Order-independent equality on the coord set: the two find forms enumerate
    // the same locked group from different starting cells, so the coord vectors
    // can differ in order while denoting the same finding.
    bool same(const LockedCandidatesFinding &o) const {
        if (unit != o.unit) return false;
        if (value != o.value) return false;
        if (coords.size() != o.coords.size()) return false;
        for (const Coord &c : coords)
            if (std::find(o.coords.begin(), o.coords.end(), c) == o.coords.end()) return false;
        return true;
    }

    void print(std::ostream &o) const override {
        o << "{";
        bool is_first = true;
        for (auto const &coord : coords) {
            if (!is_first) { o << ","; }
            is_first = false;
            o << coord;
        }
        o << "}#" << value << "[^" << tag(unit) << "]";
    }
};

// Find a locked candidate for (cell, value): all candidate cells for `value` in
// set_to_consider must also lie in set_to_ignore, and acting must actually
// eliminate something from the rest of set_to_ignore. Was
// Analyzer::find_locked_candidate; the member vector dedup is now a scan of
// `out` (this technique's own bucket, so every entry downcasts to
// LockedCandidatesFinding).
template<class Set1, class Set2>
bool find_locked_candidate(const Cell &cell, const Value &value,
                           const Set1 &set_to_consider, const Set2 &set_to_ignore, FindingList &out) {
    bool did_find = false;

    std::vector<Coord> lc_coords;
    lc_coords.push_back(cell.coord());

    for (auto const &other_cell : set_to_consider) {
        // is this a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is this the same cell?
        if (other_cell == cell) continue;

        // no! but is it a candidate for this value?
        if (!other_cell.check(value)) continue;

        // yes! but is it also a candidate?
        if (std::find(set_to_ignore.begin(), set_to_ignore.end(), other_cell) != set_to_ignore.end()) {

            // yes! record it
            lc_coords.push_back(other_cell.coord());

            // and continue the search
            continue;
        }

        // oh, this was disqualifying: we found another candidate cell in the set to ignore
        return did_find;
    }

    // ensure that this set of locked candidates, if acted on, *would* have an effect
    bool would_act = false;
    for (auto const &other_cell : set_to_ignore) {
        // is it a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it a candidate?
        if (!other_cell.check(value)) continue;

        // yes! but is it one of the locked candidates?
        if (std::find(lc_coords.begin(), lc_coords.end(), other_cell.coord()) != lc_coords.end()) continue;

        // no! we found a note cell that is in the rest of the "set_to_ignore"
        // and also is a candidate for this value: we *would* act on it
        would_act = true;
        break;
    }

    if (would_act) {
        // but is this entry already recorded?
        LockedCandidatesFinding lc(lc_coords, value, set_to_ignore.kind());
        for (auto const &f : out) {
            if (bucket_cast<LockedCandidatesFinding>(f)->same(lc)) return did_find;
        }

        // no! let's record it
        if (sVerbose) { std::cout << "  [fLC] "; lc.print(std::cout); std::cout << std::endl; }
        out.push_back(std::make_shared<LockedCandidatesFinding>(std::move(lc)));
        did_find = true;
    }

    return did_find;
}

// Apply one recorded finding to one `set`, eliminating its value from every
// cell of the set that is not itself a locked candidate. Was
// Analyzer::act_on_locked_candidate(entry, set); the board is now the passed
// reference (the member reached it through Analyzer's mBoard).
template<class Set>
bool act_on_locked_candidate(Board &board, const LockedCandidatesFinding &entry, const Set &set) {
    bool did_act = false;

    for (auto const &other_cell : set) {
        // is this a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it one of the locked candidate?
        if (std::find(entry.coords.begin(), entry.coords.end(), other_cell.coord())
                != entry.coords.end()) continue;

        // no! but is it a candidate for the locked value?
        if (!other_cell.check(entry.value)) continue;

        // yes! we'll act
        std::cout << "[LC] " << other_cell.coord() << " x" << entry.value << " [" << tag(entry.unit) << "]" << std::endl;
        board.clear_note_at(other_cell.coord(), entry.value);
        did_act = true;
    }

    return did_act;
}
} // namespace

// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
// Form 1:
// When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same row/column,
// then it is also not possible anywhere else in the same nonet
// Form 2:
// When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same nonet,
// then it is also not possible anywhere else in the same row/column
bool LockedCandidatesTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! then for each candidate value in this note cell
        for (auto const &value : cell.notes().values()) {

            // form 1
            did_find |= find_locked_candidate(cell, value, board.row(cell), board.nonet(cell), out);
            did_find |= find_locked_candidate(cell, value, board.column(cell), board.nonet(cell), out);

            // form 2
            did_find |= find_locked_candidate(cell, value, board.nonet(cell), board.row(cell), out);
            did_find |= find_locked_candidate(cell, value, board.nonet(cell), board.column(cell), out);
        }
    }

    return did_find;
}

bool LockedCandidatesTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;
    for (auto const &f : mine) {
        auto const *lc = bucket_cast<LockedCandidatesFinding>(f);

        switch (lc->unit) {
        case Unit::Row:
            did_act |= act_on_locked_candidate(board, *lc, board.row(lc->coords.at(0)));
            break;
        case Unit::Column:
            did_act |= act_on_locked_candidate(board, *lc, board.column(lc->coords.at(0)));
            break;
        case Unit::Nonet:
            did_act |= act_on_locked_candidate(board, *lc, board.nonet(lc->coords.at(0)));
            break;
        }
    }
    mine.clear();

    assert(did_act);
    return did_act;
}
