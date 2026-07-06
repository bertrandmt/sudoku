// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-xwing.h"
#include "analyzer-util.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <type_traits>

using analyzer_util::candidates;
using analyzer_util::line_of;

namespace {

// Find an X-Wing anchored on `cell` for `value`, with `cset` the base line and
// `eset` the cross line through `cell`; `csets` are all lines parallel to `cset`.
// Was Analyzer::find_xwing (the templated inner overload); records into the
// technique's own bucket `out` instead of the member vector. find() stops at the
// first hit, so out holds at most one entry (asserted below).
template<class CandidateSet, class EliminationSet>
bool find_xwing(const Board &board, const Cell &cell, const Value &value,
                const CandidateSet &cset, const EliminationSet &eset,
                const std::vector<CandidateSet> &csets, bool by_row, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));
    assert(eset.contains(cell));

    // are there exactly two candidates for this value on this candidate set?
    auto cset_candidates = candidates(cset, value);
    if (cset_candidates.size() != 2) return false;

    // yes! if cell is not the first candidate, we've already considered this cset and found it unsuitable
    if (cell != cset_candidates[0]) return false;
    const Cell& other_cell = cset_candidates[1];

    // Get the elimination set for the other cell
    const EliminationSet &other_eset = line_of<EliminationSet>(board, other_cell);
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
        auto finding = std::make_shared<XWingFinding>(value, cell.coord(), diagonal.coord(), by_row);
        assert(out.empty());
        if (sVerbose) { std::cout << "  [fXW] "; finding->print(std::cout); std::cout << std::endl; }
        out.push_back(finding);
        return true;
    }

    return false;
}

// Clear `value` from every cell of `eset` that is not one of the pattern's two
// cross-line cells. Was Analyzer::act_on_xwing (the templated inner overload).
template<class CandidateSet, class EliminationSet>
bool act_on_xwing(Board &board, const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                                    const EliminationSet &eset, Unit unit) {
    bool did_act = false;

    for (auto &cell : eset) {
        if (std::find(cset1.begin(), cset1.end(), cell) != cset1.end()) continue;
        if (std::find(cset2.begin(), cset2.end(), cell) != cset2.end()) continue;

        assert(cell.isNote());
        assert(cell.check(value));

        std::cout << "[XW] " << cell.coord() << " x" << value << " [" << tag(unit) << "]" << std::endl;
        board.clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

// Apply one recorded X-Wing: eliminate strays from both cross lines. Was
// Analyzer::act_on_xwing(const XWing &).
template<class EliminationSet>
bool act_on_xwing(Board &board, const XWingFinding &entry) {
    // The base lines and the elimination lines are always opposite kinds, so
    // derive one from the other rather than letting a caller pass a mismatched
    // pair (e.g. <Row, Row>) that would compile and silently misbehave.
    using CandidateSet = std::conditional_t<std::is_same_v<EliminationSet, Column>, Row, Column>;

    // The two base lines hold the pattern; the two elimination lines are where
    // strays for `value` get cleared. line_of picks row vs column once.
    auto anchor_candidates   = candidates(line_of<CandidateSet>(board, entry.anchor),   entry.value);
    auto diagonal_candidates = candidates(line_of<CandidateSet>(board, entry.diagonal), entry.value);
    auto anchor_eliminates   = candidates(line_of<EliminationSet>(board, entry.anchor),   entry.value);
    auto diagonal_eliminates = candidates(line_of<EliminationSet>(board, entry.diagonal), entry.value);

    // `unit` is fully determined by EliminationSet -- derive it, don't thread it.
    constexpr Unit unit = std::is_same_v<EliminationSet, Column> ? Unit::Column : Unit::Row;

    bool did_act = false;
    did_act |= act_on_xwing(board, entry.value, anchor_candidates, diagonal_candidates, anchor_eliminates, unit);
    did_act |= act_on_xwing(board, entry.value, anchor_candidates, diagonal_candidates, diagonal_eliminates, unit);
    return did_act;
}

} // namespace

bool XWingTechnique::find_xwing(const Board &board, const Cell &cell, const Value &value, FindingList &out) const {
    assert(cell.isNote());
    assert(cell.check(value));

    if (::find_xwing(board, cell, value, board.row(cell), board.column(cell), board.rows(), true, out))
        return true;
    return ::find_xwing(board, cell, value, board.column(cell), board.row(cell), board.columns(), false, out);
}

// https://www.sudokuwiki.org/x_wing_strategy
// When there are only two possible cells for a value in each of two different rows,
// and these candidates lie also in the same columns, then all other candidates for
// this value in the columns can be eliminated.
bool XWingTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        for (auto const &value : cell.notes().values()) {
            // let's see if we can anchor an X-Wing pattern in this cell for this value;
            // stop at the first one found
            if (find_xwing(board, cell, value, out)) return true;
        }
    }

    return false;
}

bool XWingTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    // Bucket invariant: every entry in this technique's bucket is an XWingFinding
    // (see NakedSingleTechnique::apply). The assert turns a wrong-bucket wiring
    // bug into a caught error, not UB.
    assert(dynamic_cast<const XWingFinding *>(mine.front().get()));
    auto const *xw = static_cast<const XWingFinding *>(mine.front().get());

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = xw->is_row_based
        ? act_on_xwing<Column>(board, *xw)
        : act_on_xwing<Row>(board, *xw);

    mine.clear();
    assert(did_act);
    return did_act;
}
