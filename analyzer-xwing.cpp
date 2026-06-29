// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "analyzer-util.h"
#include "board.h"
#include "verbose.h"
#include <algorithm>
#include <cassert>
#include <type_traits>

using analyzer_util::candidates;
using analyzer_util::line_of;

template<class CandidateSet, class EliminationSet>
bool Analyzer::find_xwing(const Cell &cell, const Value &value, const CandidateSet &cset, const EliminationSet &eset, const std::vector<CandidateSet> &csets, bool by_row) {
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
    const EliminationSet &other_eset = line_of<EliminationSet>(mBoard, other_cell);
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
        return true;
    }

    return false;
}

bool Analyzer::find_xwing(const Cell &cell, const Value &value) {
    assert(cell.isNote());
    assert(cell.check(value));

    if (find_xwing(cell, value, mBoard.row(cell), mBoard.column(cell), mBoard.rows(), true))
        return true;
    return find_xwing(cell, value, mBoard.column(cell), mBoard.row(cell), mBoard.columns(), false);
}

bool Analyzer::find_xwings() {
    // https://www.sudokuwiki.org/x_wing_strategy
    // When there are only two possible cells for a value in each of two different rows,
    // and these candidates lie also in the same columns, then all other candidates for
    // this value in the columns can be eliminated.
    for (auto const &cell: mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        for (auto const &value : cell.notes().values()) {
            // let's see if we can anchor an X-Wing pattern in this cell for this value;
            // stop at the first one found
            if (find_xwing(cell, value)) return true;
        }
    }

    return false;
}

template<class CandidateSet, class EliminationSet>
bool Analyzer::act_on_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                                const EliminationSet &eset, Unit unit) {
    bool did_act = false;

    for (auto &cell : eset) {
        if (std::find(cset1.begin(), cset1.end(), cell) != cset1.end()) continue;
        if (std::find(cset2.begin(), cset2.end(), cell) != cset2.end()) continue;

        assert(cell.isNote());
        assert(cell.check(value));

        std::cout << "[XW] " << cell.coord() << " x" << value << " [" << tag(unit) << "]" << std::endl;
        mBoard.clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

template<class EliminationSet>
bool Analyzer::act_on_xwing(const XWing &entry) {
    // The base lines and the elimination lines are always opposite kinds, so
    // derive one from the other rather than letting a caller pass a mismatched
    // pair (e.g. <Row, Row>) that would compile and silently misbehave.
    using CandidateSet = std::conditional_t<std::is_same_v<EliminationSet, Column>, Row, Column>;

    // The two base lines hold the pattern; the two elimination lines are where
    // strays for `value` get cleared. line_of picks row vs column once.
    auto anchor_candidates   = candidates(line_of<CandidateSet>(mBoard, entry.anchor),   entry.value);
    auto diagonal_candidates = candidates(line_of<CandidateSet>(mBoard, entry.diagonal), entry.value);
    auto anchor_eliminates   = candidates(line_of<EliminationSet>(mBoard, entry.anchor),   entry.value);
    auto diagonal_eliminates = candidates(line_of<EliminationSet>(mBoard, entry.diagonal), entry.value);

    // `unit` is fully determined by EliminationSet -- derive it, don't thread it.
    constexpr Unit unit = std::is_same_v<EliminationSet, Column> ? Unit::Column : Unit::Row;

    bool did_act = false;
    did_act |= act_on_xwing(entry.value, anchor_candidates, diagonal_candidates, anchor_eliminates, unit);
    did_act |= act_on_xwing(entry.value, anchor_candidates, diagonal_candidates, diagonal_eliminates, unit);
    return did_act;
}

bool Analyzer::act_on_xwing() {
    if (mXWings.empty()) return false;
    assert(mXWings.size() == 1);
    auto const entry = mXWings.back();

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = entry.is_row_based
        ? act_on_xwing<Column>(entry)
        : act_on_xwing<Row>(entry);

    mXWings.clear();
    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::XWing &xw) {
    outs << "{" << xw.anchor << "," << xw.diagonal << "}"
         << "#" << xw.value << "[^" << (xw.is_row_based ? "c" : "r") << "]";
    return outs;
}
