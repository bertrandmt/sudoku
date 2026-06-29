// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"

#include <type_traits>
#include <vector>

// Helpers shared by the fish techniques (X-Wing, Swordfish). Both are pure
// mappings over the board with no per-technique state, so they live here
// rather than being copy-pasted into each technique's translation unit. They
// are wrapped in a named namespace so these collision-prone generic names do
// not land at global scope for every TU that includes this header.
namespace analyzer_util {

// The cells of `set` that still hold `value` as a note.
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

// The Row or Column (selected by the Line type) that `x` (a Cell or Coord)
// lies on. Board exposes both row()/column() overloads for Cell and Coord, so
// one helper serves find and act, for cells and coords, with the row/column
// choice made once here instead of at every call site.
template<class Line, class CellOrCoord>
const Line &line_of(const Board &board, const CellOrCoord &x) {
    if constexpr (std::is_same_v<Line, Column>) return board.column(x);
    else                                        return board.row(x);
}

} // namespace analyzer_util
