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


void Analyzer::filter_hidden_pairs() {
    mHiddenPairs.erase(std::remove_if(mHiddenPairs.begin(), mHiddenPairs.end(),
                [this](auto const &entry) {
                    bool is_hidden_pair = test_hidden_pair(mBoard->at(entry.coords.first), mBoard->at(entry.coords.second), entry.values.first, entry.values.second);
                    if (!is_hidden_pair) {
                        if (sVerbose) std::cout << "  [xHP] " << entry << std::endl;
                    }
                    return !is_hidden_pair;
                }), mHiddenPairs.end());
}

template<class Set>
void Analyzer::find_hidden_pair(const Cell &cell, const Value &v1, const Value &v2, const Set &set) {
    assert(cell.isNote());
    assert(cell.notes().check(v1));
    assert(cell.notes().check(v2));

    // have we already recorded this cell into a pair?
    if (std::find_if(mHiddenPairs.begin(), mHiddenPairs.end(),
                [cell](auto const &entry) { return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; })
            != mHiddenPairs.end()) return;

    // no! can we find another note cell with the same pair in the same set,
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

    // did we, in fact, find another candidate?
    if (!ppair_cell) condition_met = false;

    // and also, is the candidate pair we found actionable (i.e. is it *not* a naked pair)?
    if (cell.notes().count() == 2
     && (ppair_cell && ppair_cell->notes().count() == 2)) condition_met = false;

    if (!condition_met) return; // we're done here

    // yes! let's record the entry
    HiddenPair hp(std::make_pair(cell.coord(), ppair_cell->coord()), std::make_pair(v1, v2));
    mHiddenPairs.push_back(hp);
    if (sVerbose) std::cout << "  [fHP] " << hp << std::endl;
}

template<class Set>
void Analyzer::find_hidden_pairs(Set const &set) {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row, or column,
    // and those n candidates are not possible elsewhere in that same block, row, or column, then no other
    // candidates are possible in those cells.
    // Applied for n = 2

    for (auto const &cell: set) {
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
                find_hidden_pair(cell, *pv1, *pv2, mBoard->nonet(cell));
                find_hidden_pair(cell, *pv1, *pv2, mBoard->column(cell));
                find_hidden_pair(cell, *pv1, *pv2, mBoard->row(cell));
            }
        }
    }
}

void Analyzer::find_hidden_pairs() {
    for (auto const &coord : mValueDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-revealed hidden pairs in any of this cell's blocks
        find_hidden_pairs(mBoard->nonet(cell));
        find_hidden_pairs(mBoard->column(cell));
        find_hidden_pairs(mBoard->row(cell));
    }
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-revealed hidden pairs in any of this cell's blocks
        find_hidden_pairs(mBoard->nonet(cell));
        find_hidden_pairs(mBoard->column(cell));
        find_hidden_pairs(mBoard->row(cell));
    }
}

bool Analyzer::act_on_hidden_pair(Cell &cell, const HiddenPair &entry) {
    bool acted_on_hidden_pair = false;

    auto const &v1 = entry.values.first;
    auto const &v2 = entry.values.second;

    for (auto const &value : cell.notes().values()) {
        if (value == v1) continue;
        if (value == v2) continue;

        mBoard->clear_note_at(cell.coord(), value);
        std::cout << "[HP] " << cell.coord() << " x" << value << " " << entry << std::endl;
        acted_on_hidden_pair = true;
    }

    return acted_on_hidden_pair;
}

bool Analyzer::act_on_hidden_pair() {
    bool acted_on_hidden_pair = false;

    if (mHiddenPairs.empty()) { return acted_on_hidden_pair; }

    auto const entry = mHiddenPairs.back();
    mHiddenPairs.pop_back();

    auto &c1 = mBoard->at(entry.coords.first);
    auto &c2 = mBoard->at(entry.coords.second);

    acted_on_hidden_pair |= act_on_hidden_pair(c1, entry);
    acted_on_hidden_pair |= act_on_hidden_pair(c2, entry);

    return acted_on_hidden_pair;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::HiddenPair &hp) {
    return outs << "{" << hp.coords.first << "," << hp.coords.second << "}#{" << hp.values.first << "," << hp.values.second << "}";
}
