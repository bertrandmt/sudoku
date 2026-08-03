// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-swordfish.h"
#include "analyzer-util.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <array>
#include <cassert>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

using analyzer_util::candidates;
using analyzer_util::line_of;

namespace {

// Find a Swordfish anchored on `cell` for `value`, with `cset` the first base
// line and `eset` the cross line through `cell`; `csets` are all lines parallel
// to `cset`, searched for the second and third base lines. Was
// Analyzer::find_swordfish (the templated inner overload); records into the
// technique's own bucket `out` instead of the member vector. find() stops at the
// first hit, so out holds at most one entry (asserted below).
template<class CandidateSet, class EliminationSet>
bool find_swordfish(const Board &board, const Cell &cell, const Value &value,
                    const CandidateSet &cset, const EliminationSet &eset,
                    const std::vector<CandidateSet> &csets, bool by_row, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));
    assert(eset.contains(cell));

    // Map a set of candidate cells to the elimination lines they fall on.
    auto esets_of = [&](const std::vector<Cell> &cells) {
        std::set<const EliminationSet*> s;
        for (const auto &c : cells)
            s.insert(&line_of<EliminationSet>(board, c));
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

            // Found a valid Swordfish! Record one anchor per base line: the
            // first candidate cell in each, which is what apply() maps back to
            // the base lines.
            auto finding = std::make_shared<SwordfishFinding>(
                value,
                std::array<Coord, 3>{ cset_candidates[0].coord(),
                                      cset2_candidates[0].coord(),
                                      cset3_candidates[0].coord() },
                by_row);
            assert(out.empty());
            if (sVerbose) { std::cout << "  [fSF] "; finding->print(std::cout); std::cout << std::endl; }
            out.push_back(finding);
            return true;
        }
    }

    return false;
}

// Clear `value` from every cell of `eset` that is not on one of the pattern's
// three base lines.
template<class CandidateSet, class EliminationSet>
bool act_on_swordfish(Board &board, const Value &value, const CandidateSet &cset1, const CandidateSet &cset2, const CandidateSet &cset3,
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
        board.clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

// Apply one recorded Swordfish: eliminate strays from all three cover lines.
template<class EliminationSet>
bool act_on_swordfish(Board &board, const SwordfishFinding &entry) {
    // The base lines and the elimination lines are always opposite kinds, so
    // derive one from the other rather than letting a caller pass a mismatched
    // pair (e.g. <Row, Row>) that would compile and silently misbehave.
    using CandidateSet = std::conditional_t<std::is_same_v<EliminationSet, Column>, Row, Column>;

    // The three base lines, addressed by their anchor coordinates
    const CandidateSet &l1 = line_of<CandidateSet>(board, entry.anchors[0]);
    const CandidateSet &l2 = line_of<CandidateSet>(board, entry.anchors[1]);
    const CandidateSet &l3 = line_of<CandidateSet>(board, entry.anchors[2]);

    // Collect all elimination lines where candidates appear in the three base lines
    std::set<const EliminationSet*> elims;
    for (const auto *line : {&l1, &l2, &l3})
        for (const auto &c : candidates(*line, entry.value))
            elims.insert(&line_of<EliminationSet>(board, c));

    // `unit` is fully determined by EliminationSet -- derive it, don't thread it
    // through as a second source of truth a caller could get wrong.
    constexpr Unit unit = std::is_same_v<EliminationSet, Column> ? Unit::Column : Unit::Row;

    bool did_act = false;
    for (const auto *e : elims)
        did_act |= act_on_swordfish(board, entry.value, l1, l2, l3, *e, unit);
    return did_act;
}

} // namespace

bool SwordfishTechnique::find_swordfish(const Board &board, const Cell &cell, const Value &value, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));

    // Try row-based Swordfish (eliminations in columns); if not found, try
    // column-based Swordfish (eliminations in rows)
    if (::find_swordfish(board, cell, value, board.row(cell), board.column(cell), board.rows(), true, out))
        return true;
    return ::find_swordfish(board, cell, value, board.column(cell), board.row(cell), board.columns(), false, out);
}

// https://www.sudokuwiki.org/Sword_Fish_Strategy
// Swordfish is an extension of X-Wing using three rows/columns instead of two.
// When a candidate appears 2-3 times in each of three rows (or columns),
// and all these candidates lie in the same three columns (or rows),
// then all other candidates for that value in those columns (or rows) can be eliminated.
bool SwordfishTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        for (auto const &value : cell.notes().values()) {
            // let's see if we can anchor a Swordfish pattern in this cell for this value;
            // stop at the first one found
            if (find_swordfish(board, cell, value, out)) return true;
        }
    }

    return false;
}

bool SwordfishTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    auto const &sf = bucket_cast<SwordfishFinding>(*mine.front());

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = sf.is_row_based
        ? act_on_swordfish<Column>(board, sf)
        : act_on_swordfish<Row>(board, sf);

    mine.clear();
    assert(did_act);
    return did_act;
}
