// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-swordfish.h"
#include "analyzer-fish.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "cell.h"
#include "coord.h"

#include <cassert>
#include <memory>

using analyzer_fish::act_on_plain_fish;
using analyzer_fish::anchors_of;
using analyzer_fish::find_plain_fish;

namespace {

// Record what the shared worker found as a SwordfishFinding: one anchor per base
// line, which is what apply() maps back to the base lines.
template<class Bases>
std::shared_ptr<Finding> make_finding(const Bases &b, const Value &value, bool by_row) {
    return std::make_shared<SwordfishFinding>(value, anchors_of(b), by_row);
}

} // namespace

bool SwordfishTechnique::find_swordfish(const Board &board, const Cell &cell, const Value &value, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));

    // Try row-based Swordfish (eliminations in columns); if not found, try
    // column-based Swordfish (eliminations in rows). EliminationSet cannot be
    // deduced from the argument list, so both are named -- the same call shape all
    // four fish now use.
    if (find_plain_fish<3, Row, Column>(board, cell, value, board.row(cell), board.rows(), kName,
                                        [&](const auto &b) { return make_finding(b, value, true); }, out))
        return true;
    return find_plain_fish<3, Column, Row>(board, cell, value, board.column(cell), board.columns(), kName,
                                           [&](const auto &b) { return make_finding(b, value, false); }, out);
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
        ? act_on_plain_fish<3, Column>(board, sf.value, sf.anchors, name())
        : act_on_plain_fish<3, Row>(board, sf.value, sf.anchors, name());

    mine.clear();
    assert(did_act);
    return did_act;
}
