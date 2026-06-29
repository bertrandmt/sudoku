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

    // Map a cell (or coordinate) to the Row or Column it lies on, selecting
    // which by the Line type parameter. Board exposes both row()/column()
    // overloads, so one helper serves both find and act, for cells and coords.
    template<class Line, class CellOrCoord>
    const Line &line_of(const Board &board, const CellOrCoord &x) {
        if constexpr (std::is_same_v<Line, Column>) return board.column(x);
        else                                        return board.row(x);
    }
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::find_swordfish(const Cell &cell, const Value &value, const CandidateSet &cset, const EliminationSet &eset,
                               const std::vector<CandidateSet> &csets, bool by_row) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));
    assert(eset.contains(cell));

    // Map a set of candidate cells to the elimination lines they fall on.
    auto esets_of = [&](const std::vector<Cell> &cells) {
        std::set<const EliminationSet*> s;
        for (const auto &c : cells)
            s.insert(&line_of<EliminationSet>(mBoard, c));
        return s;
    };

    // Check if the current candidate set has 2-3 candidates for this value
    auto cset_candidates = candidates(cset, value);
    if (cset_candidates.size() < 2 || cset_candidates.size() > 3) return false;

    // If cell is not the first candidate, we've already considered this cset
    if (cell != cset_candidates[0]) return false;

    // Collect elimination sets for the first candidate set
    std::set<const EliminationSet*> eset_set1 = esets_of(cset_candidates);

    // Search for second and third candidate sets
    for (auto const &cset2 : csets) {
        // Only consider subsequent csets
        if (!(cset < cset2)) continue;

        // Check if cset2 has 2-3 candidates
        auto cset2_candidates = candidates(cset2, value);
        if (cset2_candidates.size() < 2 || cset2_candidates.size() > 3) continue;

        // Collect elimination sets for second candidate set
        std::set<const EliminationSet*> eset_set2 = esets_of(cset2_candidates);

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
            std::set<const EliminationSet*> eset_set3 = esets_of(cset3_candidates);

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
            return true;
        }
    }

    return false;
}

bool Analyzer::find_swordfish(const Cell &cell, const Value &value) {
    assert(cell.isNote());
    assert(cell.check(value));

    // Try row-based Swordfish (eliminations in columns); if not found, try
    // column-based Swordfish (eliminations in rows)
    if (find_swordfish(cell, value, mBoard.row(cell), mBoard.column(cell), mBoard.rows(), true))
        return true;
    return find_swordfish(cell, value, mBoard.column(cell), mBoard.row(cell), mBoard.columns(), false);
}

bool Analyzer::find_swordfish() {
    // https://www.sudokuwiki.org/Sword_Fish_Strategy
    // Swordfish is an extension of X-Wing using three rows/columns instead of two.
    // When a candidate appears 2-3 times in each of three rows (or columns),
    // and all these candidates lie in the same three columns (or rows),
    // then all other candidates for that value in those columns (or rows) can be eliminated.

    for (auto const &cell: mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        for (auto const &value : cell.notes().values()) {
            // let's see if we can anchor a Swordfish pattern in this cell for this value;
            // stop at the first one found
            if (find_swordfish(cell, value)) return true;
        }
    }

    return false;
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::act_on_swordfish(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2, const CandidateSet &cset3,
                                                     const EliminationSet &eset, Unit unit) {
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
        std::cout << "[SF] " << cell.coord() << " x" << value << " [" << tag(unit) << "]" << std::endl;
        mBoard.clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

template<class EliminationSet>
bool Analyzer::act_on_swordfish(const Swordfish &entry) {
    // The base lines and the elimination lines are always opposite kinds, so
    // derive one from the other rather than letting a caller pass a mismatched
    // pair (e.g. <Row, Row>) that would compile and silently misbehave.
    using CandidateSet = std::conditional_t<std::is_same_v<EliminationSet, Column>, Row, Column>;

    // The three base lines, addressed by their anchor coordinates
    const CandidateSet &l1 = line_of<CandidateSet>(mBoard, entry.anchors[0]);
    const CandidateSet &l2 = line_of<CandidateSet>(mBoard, entry.anchors[1]);
    const CandidateSet &l3 = line_of<CandidateSet>(mBoard, entry.anchors[2]);

    // Collect all elimination lines where candidates appear in the three base lines
    std::set<const EliminationSet*> elims;
    for (const auto *line : {&l1, &l2, &l3})
        for (const auto &c : candidates(*line, entry.value))
            elims.insert(&line_of<EliminationSet>(mBoard, c));

    // `unit` is fully determined by EliminationSet -- derive it, don't thread it
    // through as a second source of truth a caller could get wrong.
    constexpr Unit unit = std::is_same_v<EliminationSet, Column> ? Unit::Column : Unit::Row;

    bool did_act = false;
    for (const auto *e : elims)
        did_act |= act_on_swordfish(entry.value, l1, l2, l3, *e, unit);
    return did_act;
}

bool Analyzer::act_on_swordfish() {
    if (mSwordfish.empty()) return false;
    assert(mSwordfish.size() == 1);
    auto const entry = mSwordfish.back();

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = entry.is_row_based
        ? act_on_swordfish<Column>(entry)
        : act_on_swordfish<Row>(entry);

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
