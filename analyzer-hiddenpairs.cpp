// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

bool Analyzer::test_hidden_pair(const Cell &c1, const Cell &c2, const Value &v1, const Value &v2) const {
    // are they both note cells?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;

    // yes! but do they both have at least two entries?
    if (!(c1.notes().count() >= 2 && c2.notes().count() >= 2)) return false;

    // yes! but does at least one of them have more than two candidates?
    if (!(c1.notes().count() > 2 || c2.notes().count() > 2)) return false;

    // yes! but are both values still candidates for both notes?
    if (!(c1.check(v1) && c1.check(v2)))  return false;
    if (!(c2.check(v1) && c2.check(v2)))  return false;

    // both cells in the entry are still notes with one of them having strictly
    // more than two candidates and they both have both entry values as candidates
    return true;
}

template<class Set>
bool Analyzer::find_hidden_pair(const Cell &cell, const Value &v1, const Value &v2, const Set &set) {
    assert(cell.isNote());
    assert(cell.notes().check(v1));
    assert(cell.notes().check(v2));

    bool did_find = false;

    // can we find another note cell with the same pair in the same set,
    // but no other cell with either candidate in the set?
    Cell *ppair_cell = NULL; // "the" other potential candidate
    bool condition_met = true;

    for (auto &other_cell : set) {
        // is this other cell a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it a different cell?
        if (other_cell == cell) continue;

        // yes! but are the values possible candidates for it?
        if (!other_cell.check(v1) && !other_cell.check(v2)) continue;                      // no impact on algorithm; check next cell in row
        if (other_cell.check(v1) ^ other_cell.check(v2)) { condition_met = false; break; } // either v1 or v2 is disqualified
        assert(other_cell.check(v1) && other_cell.check(v2));

        // yes! but did we already find a pair candidate cell for this hidden pair?
        if (ppair_cell) {
            condition_met = false;      // this is disqualifying: we have more than two candidates in the row
            break;
        }

        // this is "the" other candidate
        ppair_cell = &other_cell;
    }
    if (!condition_met) return did_find;

    // did we, in fact, find another candidate?
    if (!ppair_cell) return did_find;

    // yes! but is this other cell "after" this cell
    if (ppair_cell->coord() < cell.coord()) return did_find;

    // and also, is the candidate pair we found actionable (i.e. is it *not* a naked pair)?
    if (cell.notes().count() == 2 && ppair_cell->notes().count() == 2) return did_find;

    // yes! but is the entry duplicated?
    assert(cell.coord() < ppair_cell->coord());
    assert(v2 > v1);
    HiddenPair hp(std::make_pair(cell.coord(), ppair_cell->coord()), std::make_pair(v1, v2));
    if (std::find(mHiddenPairs.begin(), mHiddenPairs.end(), hp) != mHiddenPairs.end()) return did_find;

    // yes! let's record the entry
    mHiddenPairs.push_back(hp);
    if (sVerbose) std::cout << "  [fHP] " << hp << std::endl;
    did_find = true;

    return did_find;
}

bool Analyzer::find_hidden_pairs() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row, or column,
    // and those n candidates are not possible elsewhere in that same block, row, or column, then no other
    // candidates are possible in those cells.
    // Applied for n = 2
    assert(mHiddenPairs.empty());
    bool did_find = false;

    for (auto const &cell: mBoard->cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have at least two candidate values?
        auto values = cell.notes().values();
        if (!(values.size() >= 2)) continue;

        // for each pair of candidates for this cell...
        for (auto pv1 = values.begin(); pv1 != values.end(); ++pv1) {
            for (auto pv2 = pv1 + 1; pv2 != values.end(); ++pv2) {
                assert(*pv2 != *pv1);

                // let's see if we can find a hidden pair in the three cell sets
                did_find |= find_hidden_pair(cell, *pv1, *pv2, mBoard->row(cell));
                did_find |= find_hidden_pair(cell, *pv1, *pv2, mBoard->column(cell));
                did_find |= find_hidden_pair(cell, *pv1, *pv2, mBoard->nonet(cell));
            }
        }
    }

    return did_find;
}

bool Analyzer::act_on_hidden_pair(Cell &cell, const HiddenPair &entry) {
    bool did_act = false;

    auto const &v1 = entry.values.first;
    auto const &v2 = entry.values.second;

    for (auto const &value : cell.notes().values()) {
        if (value == v1) continue;
        if (value == v2) continue;

        mBoard->clear_note_at(cell.coord(), value);
        std::cout << "[HP] " << cell.coord() << " x" << value << " " << entry << std::endl;
        did_act = true;
    }

    return did_act;
}

bool Analyzer::act_on_hidden_pair() {
    bool did_act = false;

    if (mHiddenPairs.empty()) return did_act;

    for (auto const &entry : mHiddenPairs) {
        auto &c1 = mBoard->at(entry.coords.first);
        auto &c2 = mBoard->at(entry.coords.second);

        did_act |= act_on_hidden_pair(c1, entry);
        did_act |= act_on_hidden_pair(c2, entry);
    }
    mHiddenPairs.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::HiddenPair &hp) {
    return outs << "{" << hp.coords.first << "," << hp.coords.second << "}#{" << hp.values.first << "," << hp.values.second << "}";
}
