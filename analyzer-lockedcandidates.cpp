// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

namespace { // anon
template<class Set>
bool would_act_on_set(std::vector<Coord> const &coords, Value const &value, std::string const &tag, Set const &set) {
    assert(set.tag() == tag);

    bool would_act = false;

    for (auto const &cell : set) {
        // is it a note cell?
        if (!cell.isNote()) continue;

        // yes! but is it a candidate?
        if (!cell.check(value)) continue;

        // yes! but is it one of the locked candidates?
        if (std::find(coords.begin(), coords.end(), cell.coord()) != coords.end()) continue;

        would_act = true;
        break;
    }

    return would_act;
}
}

template<class Set1, class Set2>
bool Analyzer::find_locked_candidate(const Cell &cell, const Value &value, Set1 &set_to_consider, Set2 &set_to_ignore) {
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
    for (auto const &other_cell : set_to_ignore) {
        // is it a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it a candidate?
        if (!other_cell.check(value)) continue;

        // yes! but is it one of the locked candidates?
        if (std::find(lc_coords.begin(), lc_coords.end(), other_cell.coord()) != lc_coords.end()) continue;

        // no! we found a note cell that is in the rest of the "set_to_ignore"
        // and also is a candidate for this value: we *would* act on it
        LockedCandidates lc(lc_coords, value, set_to_ignore.tag());
        assert(mLockedCandidates.empty());
        mLockedCandidates.push_back(lc);
        if (sVerbose) std::cout << "  [fLC] " << lc << std::endl;
        did_find = true;
        break;
    }

    return did_find;
}

bool Analyzer::find_locked_candidates() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
    // Form 1:
    // When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same row/column,
    // then it is also not possible anywhere else in the same nonet
    // Form 2:
    // When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same nonet,
    // then it is also not possible anywhere else in the same row/column

    bool did_find = false;

    for (auto const &cell: mBoard->cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;


        // yes! then for each candidate value in this note cell
        for (auto const &value : cell.notes().values()) {

            // form 1
            did_find = find_locked_candidate(cell, value, mBoard->row(cell), mBoard->nonet(cell));
            if (!did_find) did_find = find_locked_candidate(cell, value, mBoard->column(cell), mBoard->nonet(cell));

            // form 2
            if (!did_find) did_find = find_locked_candidate(cell, value, mBoard->nonet(cell), mBoard->row(cell));
            if (!did_find) did_find = find_locked_candidate(cell, value, mBoard->nonet(cell), mBoard->column(cell));

            if (!did_find) continue;
            break;
        }

        if (!did_find) continue;
        break;
    }

    return did_find;
}

template<class Set>
bool Analyzer::act_on_locked_candidate(const LockedCandidates &entry, Set &set) {
    bool did_act = false;

    for (auto &other_cell : set) {
        // is this a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it one of the locked candidate?
        if (std::find(entry.coords.begin(), entry.coords.end(), other_cell.coord())
                != entry.coords.end()) continue;

        // no! but is it a candidate for the locked value?
        if (!other_cell.check(entry.value)) continue;

        // yes! we'll act
        std::cout << "[LC] " << other_cell.coord() << " x" << entry.value << " [" << entry.tag << "]" << std::endl;
        mBoard->clear_note_at(other_cell.coord(), entry.value);
        did_act = true;
    }

    return did_act;
}

bool Analyzer::act_on_locked_candidate() {
    bool did_act = false;

    if (mLockedCandidates.empty()) return did_act;
    assert(mLockedCandidates.size() == 1);

    auto const &entry = mLockedCandidates.back();

    switch (entry.tag[0]) {
    case 'r':
        did_act = act_on_locked_candidate(entry, mBoard->row(entry.coords.at(0)));
        break;
    case 'c':
        did_act = act_on_locked_candidate(entry, mBoard->column(entry.coords.at(0)));
        break;
    case 'n':
        did_act =  act_on_locked_candidate(entry, mBoard->nonet(entry.coords.at(0)));
        break;
    }
    mLockedCandidates.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::LockedCandidates &lc) {
    outs << "{";
    bool is_first = true;
    for (auto const &coord : lc.coords) {
        if (!is_first) { std::cout << ","; }
        is_first = false;
        outs << coord;
    }
    outs << "}#" << lc.value << "[^" << lc.tag << "]";
    return outs;
}
