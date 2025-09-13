// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

namespace { // anon
template<class Set>
bool would_act(const Set &set, const Cell &c1, const Cell &c2, const Value &v1, const Value &v2) {
    bool would_act = false;

    // can only act on a set that contains both cells
    if (!set.contains(c1)) return would_act;
    if (!set.contains(c2)) return would_act;

    for (auto const &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == c1 || other_cell == c2) continue;
        if (!other_cell.check(v1) && !other_cell.check(v2)) continue; // no impact on this cell

        would_act = true;
    }
    return would_act;
}
}

template<class Set>
bool Analyzer::test_naked_pair(const Cell &c1, const Cell &c2, const Set &set) const {
    // are these two different cells?
    if (c1 == c2) return false;

    // yes! but is c2 "after" c1?
    if (c2.coord() < c1.coord()) return false;

    // yes! but are both cells in the same set?
    if (!set.contains(c1)) return false;
    if (!set.contains(c2)) return false;

    // yes! but are both cells notes?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;

    // yes! but do both cells have only a pair of candidates?
    if (c1.notes().count() != 2) return false;
    if (c2.notes().count() != 2) return false;

    // yes! but are they the same pairs of candidates?
    auto v11 = c1.notes().values().at(0);
    auto v12 = c1.notes().values().at(1);
    auto v21 = c2.notes().values().at(0);
    auto v22 = c2.notes().values().at(1);

    if (!((v11 == v21 && v12 == v22) || (v11 == v22 && v12 == v21))) return false;

    // yes! but would acting on them have an effet?
    if (!would_act(set, c1, c2, v11, v12)) return false;

    // yes!
    return true;
}

template<class Set>
bool Analyzer::find_naked_pair(const Cell &cell, const Set &set) {
    bool did_find = false;

    for (auto const &pair_cell : set) {
        // is this candidate pair cell good?
        if (!test_naked_pair(cell, pair_cell, set)) continue;

        // yes! but is it already recorded?
        NakedPair np({cell.coord(), pair_cell.coord()}, {cell.notes().values().at(0), cell.notes().values().at(1)});
        if (std::find(mNakedPairs.begin(), mNakedPairs.end(), np) != mNakedPairs.end()) continue;

        // yes! let's record it
        mNakedPairs.push_back(np);
        if (sVerbose) std::cout << "  [fNP] " << np << std::endl;
        did_find = true;
        break;
    }

    return did_find;
}

bool Analyzer::find_naked_pairs() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row,
    // or column, and no other candidates are possible in those cells, then those n candidates
    // are not possible elsewhere in that same block, row, or column.
    // Applied for n = 2
    assert(mNakedPairs.empty());
    bool did_find = false;

    for (auto const &cell: mBoard->cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;
        // yes! but does it have only two notes?
        if (cell.notes().count() != 2) continue;

        // yes! let's see if we can find it a pair?
        did_find |= find_naked_pair(cell, mBoard->row(cell));
        did_find |= find_naked_pair(cell, mBoard->column(cell));
        did_find |= find_naked_pair(cell, mBoard->nonet(cell));
    }

    return did_find;
}

template<class Set>
bool Analyzer::act_on_naked_pair(const NakedPair &entry, Set &set) {
    bool did_act = false;

    auto const &cell1 = mBoard->at(entry.coords.first);
    auto const &cell2 = mBoard->at(entry.coords.second);

    if (!set.contains(cell2)) return did_act; // this is not the set to act on

    for (auto &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == cell1 || other_cell == cell2) continue; // not looking at either of the cell pairs

        if (other_cell.check(entry.values.first)) {
            mBoard->clear_note_at(other_cell.coord(), entry.values.first);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.first << " [" << set.tag() << "]" << std::endl;
            did_act = true;
        }
        if (other_cell.check(entry.values.second)) {
            mBoard->clear_note_at(other_cell.coord(), entry.values.second);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.second << " [" << set.tag() << "]" << std::endl;
            did_act = true;
        }
    }

    return did_act;
}

bool Analyzer::act_on_naked_pair() {
    bool did_act = false;

    if (mNakedPairs.empty()) return did_act;

    for (auto const &entry : mNakedPairs) {
        auto const &cell1 = mBoard->at(entry.coords.first);

        did_act |= act_on_naked_pair(entry, mBoard->row(cell1));
        did_act |= act_on_naked_pair(entry, mBoard->column(cell1));
        did_act |= act_on_naked_pair(entry, mBoard->nonet(cell1));
    }
    mNakedPairs.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::NakedPair &np) {
    return outs << "{" << np.coords.first << "," << np.coords.second << "}#{" << np.values.first << "," << np.values.second << "}";
}
