// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>
#include <type_traits>

namespace {
    // Helper function for Swordfish pattern detection
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
bool Analyzer::find_swordfish(const Cell &cell, const Value &value, const CandidateSet &cset, const EliminationSet &eset,
                               const std::vector<CandidateSet> &csets, bool by_row) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));
    assert(eset.contains(cell));

    bool did_find = false;

    // Check if the current candidate set has 2-3 candidates for this value
    auto cset_candidates = candidates(cset, value);
    if (cset_candidates.size() < 2 || cset_candidates.size() > 3) return did_find;

    // If cell is not the first candidate, we've already considered this cset
    if (cell != cset_candidates[0]) return did_find;

    // Collect elimination sets for the first candidate set
    std::set<const EliminationSet*> eset_set1;
    for (const auto &c : cset_candidates) {
        if constexpr (std::is_same_v<EliminationSet, Column>) {
            eset_set1.insert(&mBoard.column(c));
        } else {
            eset_set1.insert(&mBoard.row(c));
        }
    }

    // Search for second and third candidate sets
    for (auto const &cset2 : csets) {
        // Only consider subsequent csets
        if (!(cset < cset2)) continue;

        // Check if cset2 has 2-3 candidates
        auto cset2_candidates = candidates(cset2, value);
        if (cset2_candidates.size() < 2 || cset2_candidates.size() > 3) continue;

        // Collect elimination sets for second candidate set
        std::set<const EliminationSet*> eset_set2;
        for (const auto &c : cset2_candidates) {
            if constexpr (std::is_same_v<EliminationSet, Column>) {
                eset_set2.insert(&mBoard.column(c));
            } else {
                eset_set2.insert(&mBoard.row(c));
            }
        }

        // Union of elimination sets from first two candidate sets
        std::set<const EliminationSet*> eset_union = eset_set1;
        eset_union.insert(eset_set2.begin(), eset_set2.end());

        // If union already exceeds 3, can't form Swordfish
        if (eset_union.size() > 3) continue;

        // Search for third candidate set
        for (auto const &cset3 : csets) {
            // Only consider subsequent csets
            if (!(cset2 < cset3)) continue;

            // Check if cset3 has 2-3 candidates
            auto cset3_candidates = candidates(cset3, value);
            if (cset3_candidates.size() < 2 || cset3_candidates.size() > 3) continue;

            // Collect elimination sets for third candidate set
            std::set<const EliminationSet*> eset_set3;
            for (const auto &c : cset3_candidates) {
                if constexpr (std::is_same_v<EliminationSet, Column>) {
                    eset_set3.insert(&mBoard.column(c));
                } else {
                    eset_set3.insert(&mBoard.row(c));
                }
            }

            // Union of all elimination sets
            std::set<const EliminationSet*> eset_total = eset_union;
            eset_total.insert(eset_set3.begin(), eset_set3.end());

            // Must be exactly 3 elimination sets
            if (eset_total.size() != 3) continue;

            // There is something to eliminate iff some cover line holds a
            // candidate for `value` outside the three base lines. Counting ">3
            // candidates in the line" is not equivalent: a cover line hit by
            // only two of the three base lines can have exactly three
            // candidates, one of which lies outside the pattern and is
            // eliminable. The base lines here are cset/cset2/cset3, so a
            // candidate cell is "in the pattern" iff one of them contains it.
            bool has_eliminations = false;
            for (const auto* eset_ptr : eset_total) {
                for (auto const &c : *eset_ptr) {
                    if (!c.isNote() || !c.check(value)) continue;
                    if (cset.contains(c) || cset2.contains(c) || cset3.contains(c)) continue;
                    has_eliminations = true;
                    break;
                }
                if (has_eliminations) break;
            }
            if (!has_eliminations) continue;

            // Found a valid Swordfish! Record the three anchor coordinates
            Swordfish candidate_swordfish;
            candidate_swordfish.value = value;
            candidate_swordfish.anchors = {
                cset_candidates[0].coord(),
                cset2_candidates[0].coord(),
                cset3_candidates[0].coord()
            };
            candidate_swordfish.is_row_based = by_row;

            assert(mSwordfish.empty());
            mSwordfish.push_back(candidate_swordfish);
            if (sVerbose) std::cout << "  [fSF] " << candidate_swordfish << std::endl;
            did_find = true;
            break;
        }

        if (did_find) break;
    }

    return did_find;
}

bool Analyzer::find_swordfish(const Cell &cell, const Value &value) {
    assert(cell.isNote());
    assert(cell.check(value));

    bool did_find = false;

    // Try row-based Swordfish (eliminations in columns)
    did_find = find_swordfish(cell, value, mBoard.row(cell), mBoard.column(cell), mBoard.rows(), true);

    // If not found, try column-based Swordfish (eliminations in rows)
    if (!did_find) {
        did_find = find_swordfish(cell, value, mBoard.column(cell), mBoard.row(cell), mBoard.columns(), false);
    }

    return did_find;
}

bool Analyzer::find_swordfish() {
    // https://www.sudokuwiki.org/Sword_Fish_Strategy
    // Swordfish is an extension of X-Wing using three rows/columns instead of two.
    // When a candidate appears 2-3 times in each of three rows (or columns),
    // and all these candidates lie in the same three columns (or rows),
    // then all other candidates for that value in those columns (or rows) can be eliminated.

    bool did_find = false;

    for (auto const &cell: mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        auto values = cell.notes().values();
        for (auto pv = values.begin(); pv != values.end(); ++pv) {
            // let's see if we can anchor a Swordfish pattern in this cell for this value
            did_find = find_swordfish(cell, *pv);
            if (!did_find) continue;
            break;
        }

        if (!did_find) continue;
        break;
    }

    return did_find;
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::act_on_swordfish(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2, const CandidateSet &cset3,
                                                     const EliminationSet &eset, const std::string &tag) {
    bool did_act = false;

    for (auto &cell : eset) {
        // Skip cells that are not note cells
        if (!cell.isNote()) continue;

        // Skip cells that don't have this candidate
        if (!cell.check(value)) continue;

        // Skip cells that are in any of the three candidate sets
        if (cset1.contains(cell)) continue;
        if (cset2.contains(cell)) continue;
        if (cset3.contains(cell)) continue;

        // Eliminate the candidate
        std::cout << "[SF] " << cell.coord() << " x" << value << " [" << tag << "]" << std::endl;
        mBoard.clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

bool Analyzer::act_on_swordfish() {
    bool did_act = false;

    if (mSwordfish.empty()) return did_act;
    assert(mSwordfish.size() == 1);

    auto const entry = mSwordfish.back();

    if (entry.is_row_based) {
        // Row-based Swordfish: the pattern is in three rows, eliminate from columns
        auto const &row1 = mBoard.row(entry.anchors[0]);
        auto const &row2 = mBoard.row(entry.anchors[1]);
        auto const &row3 = mBoard.row(entry.anchors[2]);

        // Collect all columns where candidates appear in these three rows
        std::set<const Column*> elimination_columns;
        for (const auto &cell : row1) {
            if (cell.isNote() && cell.check(entry.value)) {
                elimination_columns.insert(&mBoard.column(cell));
            }
        }
        for (const auto &cell : row2) {
            if (cell.isNote() && cell.check(entry.value)) {
                elimination_columns.insert(&mBoard.column(cell));
            }
        }
        for (const auto &cell : row3) {
            if (cell.isNote() && cell.check(entry.value)) {
                elimination_columns.insert(&mBoard.column(cell));
            }
        }

        // Eliminate from each column
        for (const auto* col_ptr : elimination_columns) {
            did_act |= act_on_swordfish(entry.value, row1, row2, row3, *col_ptr, "c");
        }
    } else {
        // Column-based Swordfish: the pattern is in three columns, eliminate from rows
        auto const &col1 = mBoard.column(entry.anchors[0]);
        auto const &col2 = mBoard.column(entry.anchors[1]);
        auto const &col3 = mBoard.column(entry.anchors[2]);

        // Collect all rows where candidates appear in these three columns
        std::set<const Row*> elimination_rows;
        for (const auto &cell : col1) {
            if (cell.isNote() && cell.check(entry.value)) {
                elimination_rows.insert(&mBoard.row(cell));
            }
        }
        for (const auto &cell : col2) {
            if (cell.isNote() && cell.check(entry.value)) {
                elimination_rows.insert(&mBoard.row(cell));
            }
        }
        for (const auto &cell : col3) {
            if (cell.isNote() && cell.check(entry.value)) {
                elimination_rows.insert(&mBoard.row(cell));
            }
        }

        // Eliminate from each row
        for (const auto* row_ptr : elimination_rows) {
            did_act |= act_on_swordfish(entry.value, col1, col2, col3, *row_ptr, "r");
        }
    }

    mSwordfish.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::Swordfish &sf) {
    outs << "{";
    bool is_first = true;
    for (const auto &anchor : sf.anchors) {
        if (!is_first) outs << ",";
        is_first = false;
        outs << anchor;
    }
    outs << "}"
         << "#" << sf.value << "[^" << (sf.is_row_based ? "c" : "r") << "]";
    return outs;
}
