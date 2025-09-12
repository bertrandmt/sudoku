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

bool Analyzer::test_naked_pair(const Cell &c1, const Cell &c2) const {
    // are these two different cells?
    if (c1 == c2) return false;

    // yes! but are both cells in the same set?
    if (!(mBoard->row(c1) == mBoard->row(c2)
       || mBoard->column(c1) == mBoard->column(c2)
       || mBoard->nonet(c1) == mBoard->nonet(c2))) return false;

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
    if (!would_act(mBoard->row(c1), c1, c2, v11, v12)
     && !would_act(mBoard->column(c1), c1, c2, v11, v12)
     && !would_act(mBoard->nonet(c1), c1, c2, v11, v12)) return false;

    // yes!
    return true;
}

void Analyzer::filter_naked_pairs() {
    mNakedPairs.clear();
}

template<class Set>
bool Analyzer::find_naked_pair(const Cell &cell, const Set &set) {
    bool did_find = false;

    // no! but...
    for (auto const &pair_cell : set) {
        // is this candidate pair cell good?
        if (!test_naked_pair(cell, pair_cell)) continue;

        // yes! let's record it
        NakedPair np(std::make_pair(cell.coord(), pair_cell.coord()), std::make_pair(cell.notes().values().at(0), cell.notes().values().at(1)));
        assert(mNakedPairs.empty());
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

    bool did_find = false;

    for (auto const &cell: mBoard->cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;
        // yes! but does it have only two notes?
        if (cell.notes().count() != 2) continue;

        // yes! let's see if we can find it a pair?
        did_find = find_naked_pair(cell, mBoard->row(cell));
        if (!did_find) did_find = find_naked_pair(cell, mBoard->column(cell));
        if (!did_find) did_find = find_naked_pair(cell, mBoard->nonet(cell));
        if (!did_find) continue;
        break; // stop after finding one naked pair, because we're only acting on one at a time
    }

    return did_find;
}

template<class Set>
bool Analyzer::act_on_naked_pair(const NakedPair &entry, Set &set) {
    bool acted_on_naked_pair = false;

    auto const &cell1 = mBoard->at(entry.coords.first);
    auto const &cell2 = mBoard->at(entry.coords.second);

    if (!set.contains(cell2)) return acted_on_naked_pair; // this is not the set to act on

    for (auto &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == cell1 || other_cell == cell2) continue; // not looking at either of the cell pairs

        if (other_cell.check(entry.values.first)) {
            mBoard->clear_note_at(other_cell.coord(), entry.values.first);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.first << " [" << set.tag() << "]" << std::endl;
            acted_on_naked_pair = true;
        }
        if (other_cell.check(entry.values.second)) {
            mBoard->clear_note_at(other_cell.coord(), entry.values.second);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.second << " [" << set.tag() << "]" << std::endl;
            acted_on_naked_pair = true;
        }
    }

    return acted_on_naked_pair;
}

bool Analyzer::act_on_naked_pair() {
    bool acted_on_naked_pair = false;

    if (mNakedPairs.empty()) { return acted_on_naked_pair; }

    auto const entry = mNakedPairs.back();
    mNakedPairs.pop_back();
    auto const &cell1 = mBoard->at(entry.coords.first);

    acted_on_naked_pair |= act_on_naked_pair(entry, mBoard->row(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, mBoard->column(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, mBoard->nonet(cell1));

    return acted_on_naked_pair;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::NakedPair &np) {
    return outs << "{" << np.coords.first << "," << np.coords.second << "}#{" << np.values.first << "," << np.values.second << "}";
}
