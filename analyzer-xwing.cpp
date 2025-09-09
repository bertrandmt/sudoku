// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>

namespace {
    // Helper function for X-Wing pattern detection
    template<class Set>
    std::vector<Cell> candidates(const Set &set, const Value &value) {
        std::vector<Cell> candidates;
        for (auto const &cell : set) {
            if (cell.isNote() && cell.check(value)) {
                candidates.push_back(cell);
            }
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
    mXWings.erase(std::remove_if(mXWings.begin(), mXWings.end(),
                [this](const auto &entry) {
                    bool is_xwing;
                    if (entry.is_row_based) {
                        is_xwing = test_xwing(entry.value,
                            mBoard->row(entry.anchor), mBoard->row(entry.diagonal),
                            mBoard->column(entry.anchor), mBoard->column(entry.diagonal));
                    } else {
                        is_xwing = test_xwing(entry.value,
                            mBoard->column(entry.anchor), mBoard->column(entry.diagonal),
                            mBoard->row(entry.anchor), mBoard->row(entry.diagonal));
                    }
                    if (!is_xwing) {
                        if (sVerbose) std::cout << "  [xXW] " << entry << std::endl;
                    }
                    return !is_xwing;
                }), mXWings.end());
}


void Analyzer::find_xwing_by_row(const Cell &cell, const Value &value) {
    // Try to find row-based X-Wings: look for another row that has candidates at the same columns
    // Only check rows with higher indices to avoid duplicates
    const auto& row = mBoard->row(cell);

    // are there exactly two candidates for this value on this row?
    auto row_candidates = candidates(row, value);
    if (row_candidates.size() != 2) return;

    // yes! identify which cell is which
    assert(row_candidates[0] == cell || row_candidates[1] == cell);
    const Cell& other_candidate = (row_candidates[0] == cell) ? row_candidates[1] : row_candidates[0];

    // is the cell in canonical position?
    if (mBoard->column(other_candidate) < mBoard->column(cell)) return;

    // yes! for every subsequent row
    for (auto other_row_it = mBoard->rows().begin() + row.index() + 1; other_row_it != mBoard->rows().end(); ++other_row_it) {
        const Row& other_row = *other_row_it;

        // are there exactly two candidates for this value on this row?
        auto other_row_candidates = candidates(other_row, value);
        if (other_row_candidates.size() != 2) continue;

        // yes! does this other row have candidates in the same columns as our row
        if (!((mBoard->column(cell).contains(other_row_candidates[0]) && mBoard->column(other_candidate).contains(other_row_candidates[1])) ||
              (mBoard->column(cell).contains(other_row_candidates[1]) && mBoard->column(other_candidate).contains(other_row_candidates[0])))) continue;

        // yes! identify the diagonal cell (other candidate in other_row that's in other_candidate's column)
        const Cell& diagonal = mBoard->column(other_candidate).contains(other_row_candidates[0]) ?
                               other_row_candidates[0] : other_row_candidates[1];

        // is this a valid XWing pattern (i.e. other candidates in the same column would be eliminated)?
        auto anchor_col_candidates = candidates(mBoard->column(cell), value);
        auto diagonal_col_candidates = candidates(mBoard->column(diagonal), value);
        if (anchor_col_candidates.size() <= 2 && diagonal_col_candidates.size() <= 2) continue;

        // yes! is the pattern already recorded (somehow)
        XWing candidate_xwing{value, cell.coord(), diagonal.coord(), true};
        if (std::find(mXWings.begin(), mXWings.end(), candidate_xwing) != mXWings.end()) continue;

        // no! let's record it
        mXWings.push_back(candidate_xwing);
        if (sVerbose) std::cout << "  [fXW] " << candidate_xwing << std::endl;
    }
}

void Analyzer::find_xwing_by_column(const Cell &cell, const Value &value) {
    // Try to find column-based X-Wings: look for another column that has candidates at the same rows
    // Only check columns with higher indices to avoid duplicates
    const auto& column = mBoard->column(cell);

    // are there exactly two candidates for this value in this column?
    auto col_candidates = candidates(column, value);
    if (col_candidates.size() != 2) return;

    // yes! identify which cell is which
    assert(col_candidates[0] == cell || col_candidates[1] == cell);
    const Cell& other_candidate = (col_candidates[0] == cell) ? col_candidates[1] : col_candidates[0];

    // is the cell in canonical position?
    if (mBoard->row(other_candidate) < mBoard->row(cell)) return;

    // yes! for every subsequent column
    for (auto other_col_it = mBoard->columns().begin() + column.index() + 1; other_col_it != mBoard->columns().end(); ++other_col_it) {
        const Column& other_col = *other_col_it;

        // are there exactly two candidates for this value in this column?
        auto other_col_candidates = candidates(other_col, value);
        if (other_col_candidates.size() != 2) continue;

        // yes! does this other column have candidates in the same rows as our column
        if (!((mBoard->row(cell).contains(other_col_candidates[0]) && mBoard->row(other_candidate).contains(other_col_candidates[1])) ||
              (mBoard->row(cell).contains(other_col_candidates[1]) && mBoard->row(other_candidate).contains(other_col_candidates[0])))) continue;

        // yes! identify the diagonal cell (other candidate in other_col that's in other_candidate's row)
        const Cell& diagonal = mBoard->row(other_candidate).contains(other_col_candidates[0]) ?
                               other_col_candidates[0] : other_col_candidates[1];

        // is this a valid XWing pattern (i.e. other candidates in the same row would be eliminated)?
        auto anchor_row_candidates = candidates(mBoard->row(cell), value);
        auto diagonal_row_candidates = candidates(mBoard->row(diagonal), value);
        if (anchor_row_candidates.size() <= 2 && diagonal_row_candidates.size() <= 2) continue;

        // yes! is the pattern already recorded (somehow)
        XWing candidate_xwing{value, cell.coord(), diagonal.coord(), false};
        if (std::find(mXWings.begin(), mXWings.end(), candidate_xwing) != mXWings.end()) continue;

        // no! let's record it
        mXWings.push_back(candidate_xwing);
        if (sVerbose) std::cout << "  [fXW] " << candidate_xwing << std::endl;
    }
}

void Analyzer::find_xwing(const Cell &cell, const Value &value) {
    assert(cell.isNote());
    assert(cell.check(value));

    find_xwing_by_row(cell, value);
    find_xwing_by_column(cell, value);
}

template<class Set>
void Analyzer::find_xwing(Set const &set) {
    // https://www.sudokuwiki.org/x_wing_strategy
    // When there are only two possible cells for a value in each of two different rows,
    // and these candidates lie also in the same columns, then all other candidates for
    // this value in the columns can be eliminated.

    for (auto const &cell: set) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        auto values = cell.notes().values();
        for (auto pv = values.begin(); pv != values.end(); ++pv) {
            // is this cell already recorded in an X-Wing patter for this value?
            if (std::find_if(mXWings.begin(), mXWings.end(),
                        [cell, pv](auto const &entry) {
                            if (*pv != entry.value) return false;
                            const Coord& c = cell.coord();
                            // Check if cell is one of the four corners of the XWing
                            return c == entry.anchor ||
                                   c == entry.diagonal ||
                                   c == Coord(entry.anchor.row(), entry.diagonal.column()) ||
                                   c == Coord(entry.diagonal.row(), entry.anchor.column()); })
                    != mXWings.end()) continue;

            // let's see if we can anchor an X-Wing pattern in this cell for this value
            find_xwing(cell, *pv);
        }
    }
}

void Analyzer::find_xwings() {
    for (auto const &coord : mValueDirtySet) {
        // are there now-revealed X-Wing patterns anchored in any of this cell's blocks
        find_xwing(mBoard->nonet(coord));
        find_xwing(mBoard->column(coord));
        find_xwing(mBoard->row(coord));
    }

    for (auto const &coord : mNotesDirtySet) {
        // are there now-revealed hidden pairs in any of this cell's blocks
        find_xwing(mBoard->nonet(coord));
        find_xwing(mBoard->column(coord));
        find_xwing(mBoard->row(coord));
    }
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::act_on_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                                const EliminationSet &eset, const std::string &tag) {
    bool acted = false;

    for (auto &cell : eset) {
        if (std::find(cset1.begin(), cset1.end(), cell) != cset1.end()) continue;
        if (std::find(cset2.begin(), cset2.end(), cell) != cset2.end()) continue;

        assert(cell.isNote());
        assert(cell.check(value));

        std::cout << "[XW] " << cell.coord() << " x" << value << " [" << tag << "]" << std::endl;
        mBoard->clear_note_at(cell.coord(), value);
        acted = true;
    }

    return acted;
}

bool Analyzer::act_on_xwing() {
    if (mXWings.empty()) { return false; }

    auto const entry = mXWings.back();
    mXWings.pop_back();

    bool acted = false;

    if (entry.is_row_based) {
        // Row-based X-Wing: eliminate candidates from the two columns
        auto anchor_row_candidates = candidates(mBoard->row(entry.anchor), entry.value);
        auto diagonal_row_candidates = candidates(mBoard->row(entry.diagonal), entry.value);
        auto anchor_column_eliminates = candidates(mBoard->column(entry.anchor), entry.value);
        auto diagonal_column_eliminates = candidates(mBoard->column(entry.diagonal), entry.value);

        acted |= act_on_xwing(entry.value, anchor_row_candidates, diagonal_row_candidates,
                                           anchor_column_eliminates, "c");
        acted |= act_on_xwing(entry.value, anchor_row_candidates, diagonal_row_candidates,
                                           diagonal_column_eliminates, "c");
    } else {
        // Column-based X-Wing: eliminate candidates from the two rows
        auto anchor_column_candidates = candidates(mBoard->column(entry.anchor), entry.value);
        auto diagonal_column_candidates = candidates(mBoard->column(entry.diagonal), entry.value);
        auto anchor_row_eliminates = candidates(mBoard->row(entry.anchor), entry.value);
        auto diagonal_row_eliminates = candidates(mBoard->row(entry.diagonal), entry.value);

        acted |= act_on_xwing(entry.value, anchor_column_candidates, diagonal_column_candidates,
                                           anchor_row_eliminates, "r");
        acted |= act_on_xwing(entry.value, anchor_column_candidates, diagonal_column_candidates,
                                           diagonal_row_eliminates, "r");
    }

    return acted;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::XWing &xw) {
    outs << "{" << xw.anchor << "," << xw.diagonal << "}"
         << "#" << xw.value << "[^" << (xw.is_row_based ? "c" : "r") << "]";
    return outs;
}
