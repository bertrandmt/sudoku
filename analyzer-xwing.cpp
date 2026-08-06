// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-xwing.h"
#include "analyzer-fish.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "cell.h"
#include "coord.h"

#include <array>
#include <cassert>
#include <memory>

using analyzer_fish::act_on_plain_fish;
using analyzer_fish::find_plain_fish;

namespace {

// Record what the shared worker found as an XWingFinding: the two opposite
// corners of the rectangle.
//
// X-Wing predates the one-anchor-per-base-line convention the other three fish
// record with, and its print format is documented output, so it keeps its own
// pair. The anchor is the first candidate of the first base line and the diagonal
// the *second* candidate of the second; candidates() walks a line in ascending
// order, so that is the far corner rather than merely another cell of the
// pattern. apply() recovers the base lines from the two of them together, one
// each, exactly as the other fish recover theirs from one anchor per line.
template<class Bases>
std::shared_ptr<Finding> make_finding(const Bases &b, const Value &value, bool by_row) {
    // Confinement to two cover lines, plus the floor of two candidates per base
    // line, leaves each base line with exactly two candidates -- so the far
    // corner is at [1], and this is an invariant of the search rather than a
    // property of this particular board.
    assert(b.candidates[0].size() == 2);
    assert(b.candidates[1].size() == 2);

    return std::make_shared<XWingFinding>(value, b.candidates[0][0].coord(),
                                                 b.candidates[1][1].coord(), by_row);
}

} // namespace

bool XWingTechnique::find_xwing(const Board &board, const Cell &cell, const Value &value, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));

    // Try row-based (eliminations in columns); if not found, try column-based
    // (eliminations in rows). EliminationSet cannot be deduced from the argument
    // list, so both are named -- the same call shape all four fish now use.
    if (find_plain_fish<2, Row, Column>(board, cell, value, board.row(cell), board.rows(), kName,
                                        [&](const auto &b) { return make_finding(b, value, true); }, out))
        return true;
    return find_plain_fish<2, Column, Row>(board, cell, value, board.column(cell), board.columns(), kName,
                                           [&](const auto &b) { return make_finding(b, value, false); }, out);
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

    auto const &xw = bucket_cast<XWingFinding>(*mine.front());
    // One corner per base line, which is what the shared worker needs to recover
    // them; see make_finding above.
    const std::array<Coord, 2> anchors{ xw.anchor, xw.diagonal };

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = xw.is_row_based
        ? act_on_plain_fish<2, Column>(board, xw.value, anchors, name())
        : act_on_plain_fish<2, Row>(board, xw.value, anchors, name());

    mine.clear();
    assert(did_act);
    return did_act;
}
