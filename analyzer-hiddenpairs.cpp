// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

#include <algorithm>

template<class Set>
bool Analyzer::test_hidden_pair(const Cell &c1, const Cell &c2,
                                const Value &v1, const Value &v2, const Set &set) const {
    // are these two different cells, carrying two different values?
    if (c1 == c2) return false;
    if (v1 == v2) return false;

    // yes! but is v2 strictly "after" v1? (find_ enumerates v1<v2; the friend
    // hook can pass them either way, so the predicate guards the precondition
    // itself. The v1==v2 case above is the other half of that guard.)
    if (v2 < v1) return false;

    // yes! but is c2 "after" c1?
    if (c2.coord() < c1.coord()) return false;

    // yes! but are both cells in the same set?
    if (!set.contains(c1)) return false;
    if (!set.contains(c2)) return false;

    // yes! but are both cells notes?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;

    // yes! but do both cells carry the candidate pair?
    if (!c1.check(v1) || !c1.check(v2)) return false;
    if (!c2.check(v1) || !c2.check(v2)) return false;

    // yes! but is the pair "hidden", i.e. no other cell in the set carries
    // either value? (A stray cell carrying exactly one value, or a third cell
    // carrying both, are equally disqualifying -- this one test subsumes both
    // rejection paths the old discovery scan handled separately.)
    for (auto const &other_cell : set) {
        if (!other_cell.isNote()) continue;
        if (other_cell == c1 || other_cell == c2) continue;
        if (other_cell.check(v1) || other_cell.check(v2)) return false;
    }

    // yes! but is it actionable (i.e. *not* a naked pair, with nothing else to strip)?
    if (c1.notes().count() == 2 && c2.notes().count() == 2) return false;

    // yes!
    return true;
}

template<class Set>
bool Analyzer::find_hidden_pair(const Cell &cell, const Value &v1, const Value &v2, const Set &set) {
    for (auto const &other_cell : set) {
        // is this candidate hidden pair good?
        if (!test_hidden_pair(cell, other_cell, v1, v2, set)) continue;

        // yes! but is it already recorded?
        HiddenPair hp({cell.coord(), other_cell.coord()}, {v1, v2});
        if (std::find(mHiddenPairs.begin(), mHiddenPairs.end(), hp) != mHiddenPairs.end()) continue;

        // no! let's record it
        mHiddenPairs.push_back(hp);
        if (sVerbose) std::cout << "  [fHP] " << hp << std::endl;
        return true;
    }

    return false;
}

bool Analyzer::find_hidden_pairs() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row, or column,
    // and those n candidates are not possible elsewhere in that same block, row, or column, then no other
    // candidates are possible in those cells.
    // Applied for n = 2
    assert(mHiddenPairs.empty());
    bool did_find = false;

    for (auto const &cell: mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have at least two candidate values?
        auto values = cell.notes().values();
        if (values.size() < 2) continue;

        // for each pair of candidates for this cell...
        for (auto pv1 = values.begin(); pv1 != values.end(); ++pv1) {
            for (auto pv2 = pv1 + 1; pv2 != values.end(); ++pv2) {
                assert(*pv2 != *pv1);

                // let's see if we can find a hidden pair in the three cell sets
                did_find |= find_hidden_pair(cell, *pv1, *pv2, mBoard.row(cell));
                did_find |= find_hidden_pair(cell, *pv1, *pv2, mBoard.column(cell));
                did_find |= find_hidden_pair(cell, *pv1, *pv2, mBoard.nonet(cell));
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

        mBoard.clear_note_at(cell.coord(), value);
        std::cout << "[HP] " << cell.coord() << " x" << value << " " << entry << std::endl;
        did_act = true;
    }

    return did_act;
}

bool Analyzer::act_on_hidden_pair() {
    bool did_act = false;

    if (mHiddenPairs.empty()) return did_act;

    for (auto const &entry : mHiddenPairs) {
        auto &c1 = mBoard.at(entry.coords.first);
        auto &c2 = mBoard.at(entry.coords.second);

        did_act |= act_on_hidden_pair(c1, entry);
        did_act |= act_on_hidden_pair(c2, entry);
    }
    mHiddenPairs.clear();

    assert(did_act);
    return did_act;
}

// Explicit instantiation so the whitebox test (tests/unit/test_analyzer.cpp)
// can link test_hidden_pair<Row> directly. Its only in-TU caller is
// find_hidden_pair, which at -O3 g++ inlines, emitting no out-of-line copy of
// the predicate; the external reference from the test TU then fails to link
// (clang happens to keep a weak definition, so it only bit the gcc build).
template bool Analyzer::test_hidden_pair<Row>(const Cell &, const Cell &, const Value &, const Value &, const Row &) const;

std::ostream& operator<<(std::ostream& outs, const Analyzer::HiddenPair &hp) {
    return outs << "{" << hp.coords.first << "," << hp.coords.second << "}#{" << hp.values.first << "," << hp.values.second << "}";
}
