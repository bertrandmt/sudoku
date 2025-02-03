// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

namespace { // anon
template<class Set>
bool would_act(const Set &set, const Cell &c1, const Cell &c2, const Value &v1, const Value &v2) {
    bool would_act = false;
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
    if (!(mBoard->nonet(c1) == mBoard->nonet(c2)
       || mBoard->column(c1) == mBoard->column(c2)
       || mBoard->row(c1) == mBoard->row(c2))) return false;

    // yes! but are both cells notes?
    if (!c1.isNote() || !c2.isNote()) return false;

    // yes! but do both cells have only a pair of candidates?
    if (c1.notes().count() != 2 || c2.notes().count() != 2) return false;

    // yes! but are they the same pairs of candidates?
    auto v11 = c1.notes().values().at(0);
    auto v12 = c1.notes().values().at(1);
    auto v21 = c2.notes().values().at(0);
    auto v22 = c2.notes().values().at(1);

    if (!((v11 == v21 && v12 == v22) || (v11 == v22 && v12 == v21))) return false;

    // yes! but would acting on them have an effet?
    if (mBoard->nonet(c1)  == mBoard->nonet(c2)  && !would_act(mBoard->nonet(c1), c1, c2, v11, v12)) return false;
    if (mBoard->column(c1) == mBoard->column(c2) && !would_act(mBoard->column(c1), c1, c2, v11, v12)) return false;
    if (mBoard->row(c1)    == mBoard->row(c2)    && !would_act(mBoard->row(c1), c1, c2, v11, v12)) return false;

    // yes!
    return true;
}

void Analyzer::filter_naked_pairs() {
    mNakedPairs.erase(std::remove_if(mNakedPairs.begin(), mNakedPairs.end(),
                [this](const auto &entry) {
                    bool is_naked_pair = test_naked_pair(mBoard->at(entry.coords.first), mBoard->at(entry.coords.second));
                    if (!is_naked_pair) {
                        if (sVerbose) std::cout << "  [xNP] " << entry << std::endl;
                    }
                    return !is_naked_pair;
                }), mNakedPairs.end());
}

template<class Set>
void Analyzer::find_naked_pair(const Cell &cell, const Set &set) {
    // have we already recorded this pair?
    if (std::find_if(mNakedPairs.begin(), mNakedPairs.end(),
                [cell](auto const &entry) { return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; })
            != mNakedPairs.end()) return;

    // no! but...
    for (auto const &pair_cell : set) {
        // is this candidate pair cell good?
        if (!test_naked_pair(cell, pair_cell)) continue;

        // yes! let's record it
        NakedPair np(std::make_pair(cell.coord(), pair_cell.coord()), std::make_pair(cell.notes().values().at(0), cell.notes().values().at(1)));
        mNakedPairs.push_back(np);
        if (sVerbose) std::cout << "  [fNP] " << np << std::endl;
        break;
    }
}

void Analyzer::find_naked_pairs() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row,
    // or column, and no other candidates are possible in those cells, then those n candidates
    // are not possible elsewhere in that same block, row, or column.
    // Applied for n = 2
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // is this a note cell?
        if (!cell.isNote()) continue;
        // yes! but does it have only two notes?
        if (cell.notes().count() != 2) continue;

        // yes! let's see if we can find it a pair?
        find_naked_pair(cell, mBoard->nonet(cell));
        find_naked_pair(cell, mBoard->column(cell));
        find_naked_pair(cell, mBoard->row(cell));
    }
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

        bool acted_on_other_cell = false;
        if (other_cell.check(entry.values.first)) {
            mBoard->clear_note_at(other_cell.coord(), entry.values.first);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.first << " [" << set.tag() << "]" << std::endl;
            acted_on_other_cell = true;
        }
        if (other_cell.check(entry.values.second)) {
            mBoard->clear_note_at(other_cell.coord(), entry.values.second);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.second << " [" << set.tag() << "]" << std::endl;
            acted_on_other_cell = true;
        }

        if (acted_on_other_cell) {
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

    acted_on_naked_pair |= act_on_naked_pair(entry, mBoard->nonet(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, mBoard->column(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, mBoard->row(cell1));

    return acted_on_naked_pair;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::NakedPair &np) {
    return outs << "{" << np.coords.first << "," << np.coords.second << "}#{" << np.values.first << "," << np.values.second << "}";
}
