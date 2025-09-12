// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>
#include <type_traits>

namespace {
    // Helper function for X-Wing pattern detection
    template<class Set>
    std::vector<Cell> candidates(const Set &set, const Value &value) {
        std::vector<Cell> candidates;

        for (auto const &cell : set) {
            if (!cell.isNote()) continue;
            if (!cell.check(value)) continue;

            candidates.push_back(cell);
        }

        return candidates;
    }
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::test_xwing(const Value &value, const CandidateSet &cset1,   const CandidateSet &cset2,
                                              const EliminationSet &eset1, const EliminationSet &eset2) {
    // are there exactly two candidates for value in cset1?
    auto candidates1 = candidates(cset1, value);
    if (candidates1.size() != 2) return false;

    // yes, but are there exactly two candidates for value in cset2?
    auto candidates2 = candidates(cset2, value);
    if (candidates2.size() != 2) return false;

    // yes, but are there at least two candidates in both eset1 and eset2?
    auto eliminates1 = candidates(eset1, value);
    auto eliminates2 = candidates(eset2, value);
    if (eliminates1.size() < 2 || eliminates2.size() < 2) return false;

    // yes, but are there more than two candidates in eset1 or eset 2?
    if (eliminates1.size() <= 2 && eliminates2.size() <= 2) return false;

    // yes, but does eliminates1 contain the first cell in candidates1?
    if (std::find(eliminates1.begin(), eliminates1.end(), candidates1[0]) == eliminates1.end()) return false;

    // yes, but does eliminates2 contain the second cell in candidates1?
    if (std::find(eliminates2.begin(), eliminates2.end(), candidates1[1]) == eliminates2.end()) return false;

    // yes, but does eliminates1 contain the first cell in candidates2?
    if (std::find(eliminates1.begin(), eliminates1.end(), candidates2[0]) == eliminates1.end()) return false;

    // yes, but does eliminates2 contain the second cell in candidates2?
    if (std::find(eliminates2.begin(), eliminates2.end(), candidates2[1]) == eliminates2.end()) return false;

    return true;
}


void Analyzer::filter_xwings() {
    mXWings.clear();
}


template<class CandidateSet, class EliminationSet>
bool Analyzer::find_xwing(const Cell &cell, const Value &value, const CandidateSet &cset, const EliminationSet &eset, const std::vector<CandidateSet> &csets, bool by_row) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));
    assert(eset.contains(cell));

    bool did_find = false;

    // are there exactly two candidates for this value on this candidate set?
    auto cset_candidates = candidates(cset, value);
    if (cset_candidates.size() != 2) return did_find;

    // yes! if cell is not the first candidate, we've already considered this cset and found it unsuitable
    if (cell != cset_candidates[0]) return did_find;
    const Cell& other_cell = cset_candidates[1];

    // Get the elimination set for the other cell
    const EliminationSet &other_eset = [&]() -> const EliminationSet& {
        if constexpr (std::is_same_v<EliminationSet, Column>) {
            return mBoard->column(other_cell);
        } else {
            return mBoard->row(other_cell);
        }
    }();
    assert(eset < other_eset);

    // yes! for every subsequent cset
    for (auto const &other_cset : csets) {
        // subsequent csets only
        if (!(cset < other_cset)) continue;

        // are there exactly two candidates for this value on this other cset?
        auto other_cset_candidates = candidates(other_cset, value);
        if (other_cset_candidates.size() != 2) continue;

        // yes! does this other cset have candidates in the same esets as our cset
        assert(!(eset.contains(other_cset_candidates[1]) && other_eset.contains(other_cset_candidates[0])));
        if (!eset.contains(other_cset_candidates[0])) continue;
        if (!other_eset.contains(other_cset_candidates[1])) continue;

        // yes! identify the diagonal cell
        const Cell& diagonal = other_cset_candidates[1];
        assert(other_eset.contains(diagonal));

        // is this a valid XWing pattern (i.e. other candidates in the same esets would be eliminated)?
        auto anchor_eliminates = candidates(eset, value);
        auto diagonal_eliminates = candidates(other_eset, value);
        if (anchor_eliminates.size() <= 2 && diagonal_eliminates.size() <= 2) continue;

        // yes! record the pattern
        XWing candidate_xwing{value, cell.coord(), diagonal.coord(), by_row};
        assert(mXWings.empty());
        mXWings.push_back(candidate_xwing);
        if (sVerbose) std::cout << "  [fXW] " << candidate_xwing << std::endl;
        did_find = true;
        break;
    }

    return did_find;
}

bool Analyzer::find_xwing(const Cell &cell, const Value &value) {
    assert(cell.isNote());
    assert(cell.check(value));

    bool did_find = false;

    did_find = find_xwing(cell, value, mBoard->row(cell), mBoard->column(cell), mBoard->rows(), true);
    if (!did_find) did_find = find_xwing(cell, value, mBoard->column(cell), mBoard->row(cell), mBoard->columns(), false);

    return did_find;
}

bool Analyzer::find_xwings() {
    // https://www.sudokuwiki.org/x_wing_strategy
    // When there are only two possible cells for a value in each of two different rows,
    // and these candidates lie also in the same columns, then all other candidates for
    // this value in the columns can be eliminated.
    bool did_find = false;

    for (auto const &cell: mBoard->cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        auto values = cell.notes().values();
        for (auto pv = values.begin(); pv != values.end(); ++pv) {
            // let's see if we can anchor an X-Wing pattern in this cell for this value
            did_find = find_xwing(cell, *pv);
            if (!did_find) continue;
            break;
        }

        if (!did_find) continue;
        break;
    }

    return did_find;
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::act_on_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                                const EliminationSet &eset, const std::string &tag) {
    bool did_act = false;

    for (auto &cell : eset) {
        if (std::find(cset1.begin(), cset1.end(), cell) != cset1.end()) continue;
        if (std::find(cset2.begin(), cset2.end(), cell) != cset2.end()) continue;

        assert(cell.isNote());
        assert(cell.check(value));

        std::cout << "[XW] " << cell.coord() << " x" << value << " [" << tag << "]" << std::endl;
        mBoard->clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

bool Analyzer::act_on_xwing() {
    bool did_act = false;

    if (mXWings.empty()) return did_act;
    assert(mXWings.size() == 1);

    auto const entry = mXWings.back();

    if (entry.is_row_based) {
        // Row-based X-Wing: eliminate candidates from the two columns
        auto anchor_row_candidates = candidates(mBoard->row(entry.anchor), entry.value);
        auto diagonal_row_candidates = candidates(mBoard->row(entry.diagonal), entry.value);
        auto anchor_column_eliminates = candidates(mBoard->column(entry.anchor), entry.value);
        auto diagonal_column_eliminates = candidates(mBoard->column(entry.diagonal), entry.value);

        did_act |= act_on_xwing(entry.value, anchor_row_candidates, diagonal_row_candidates,
                                           anchor_column_eliminates, "c");
        did_act |= act_on_xwing(entry.value, anchor_row_candidates, diagonal_row_candidates,
                                           diagonal_column_eliminates, "c");
    } else {
        // Column-based X-Wing: eliminate candidates from the two rows
        auto anchor_column_candidates = candidates(mBoard->column(entry.anchor), entry.value);
        auto diagonal_column_candidates = candidates(mBoard->column(entry.diagonal), entry.value);
        auto anchor_row_eliminates = candidates(mBoard->row(entry.anchor), entry.value);
        auto diagonal_row_eliminates = candidates(mBoard->row(entry.diagonal), entry.value);

        did_act |= act_on_xwing(entry.value, anchor_column_candidates, diagonal_column_candidates,
                                           anchor_row_eliminates, "r");
        did_act |= act_on_xwing(entry.value, anchor_column_candidates, diagonal_column_candidates,
                                           diagonal_row_eliminates, "r");
    }

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::XWing &xw) {
    outs << "{" << xw.anchor << "," << xw.diagonal << "}"
         << "#" << xw.value << "[^" << (xw.is_row_based ? "c" : "r") << "]";
    return outs;
}
