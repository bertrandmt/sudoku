// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-finnedswordfish.h"
#include "analyzer-fish.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "cell.h"
#include "coord.h"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

using analyzer_fish::act_on_finned_fish;
using analyzer_fish::anchors_of;
using analyzer_fish::find_finned_fish;

namespace {

// Record what the shared worker found as a FinnedSwordfishFinding: one anchor per
// base line, plus the fins in the order the worker discovered them.
template<class Bases>
std::shared_ptr<Finding> make_finding(const Bases &b, std::vector<Coord> fins,
                                      const Value &value, bool by_row) {
    return std::make_shared<FinnedSwordfishFinding>(value, anchors_of(b), std::move(fins), by_row);
}

} // namespace

bool FinnedSwordfishTechnique::find_finned_swordfish(const Board &board, const Cell &cell, const Value &value, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));

    // Try row-based (eliminations in columns); if not found, try column-based
    // (eliminations in rows). EliminationSet cannot be deduced from the argument
    // list -- the cover is chosen, not handed in -- so both are named.
    if (find_finned_fish<3, Row, Column>(board, cell, value, board.row(cell), board.rows(), kName,
            [&](const auto &b, auto fins) { return make_finding(b, std::move(fins), value, true); }, out))
        return true;
    return find_finned_fish<3, Column, Row>(board, cell, value, board.column(cell), board.columns(), kName,
            [&](const auto &b, auto fins) { return make_finding(b, std::move(fins), value, false); }, out);
}

// https://www.sudokuwiki.org/Finned_Swordfish
// A finned Swordfish is a Swordfish whose base lines are allowed extra candidates
// outside the three cover lines, so long as all of those fins share one nonet. The
// value is then eliminated from the cover lines' cells inside that nonet, outside
// the base lines.
bool FinnedSwordfishTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        for (auto const &value : cell.notes().values()) {
            // let's see if we can anchor a finned Swordfish in this cell for this
            // value; stop at the first one found
            if (find_finned_swordfish(board, cell, value, out)) return true;
        }
    }

    return false;
}

bool FinnedSwordfishTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    auto const &fs = bucket_cast<FinnedSwordfishFinding>(*mine.front());

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = fs.is_row_based
        ? act_on_finned_fish<3, Column>(board, fs.value, fs.anchors, fs.fins, name())
        : act_on_finned_fish<3, Row>(board, fs.value, fs.anchors, fs.fins, name());

    mine.clear();
    assert(did_act);
    return did_act;
}
