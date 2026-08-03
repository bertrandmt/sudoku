// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value, Cell used in the finding and the test_ contract below

#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

// Simple Coloring (a.k.a. Single's Chains): for one value, build a graph of its
// candidate cells linked by bi-location (conjugate) strong links, two-color it,
// and eliminate via Rule 2 (a color repeated in a unit is false) or Rule 4 (an
// off-chain candidate that sees both colors is false).
//
// SC is *materialized-object* shaped (docs/test-predicate-idiom.md): building
// the graph and scoring its eliminations are distinct steps, so it carries a
// separable test_color_chain predicate. ColorChainFinding is in this header, not
// file-local, because the whitebox cases construct one and hand it to that
// predicate.
struct ColorChainFinding : Finding {
    Value value;
    std::unordered_map<Coord, bool> cells;  // coord -> color (true=green, false=red)

    explicit ColorChainFinding(Value v) : value(v) { }

    // Both vectors come back sorted by coord, for the same reason print() sorts:
    // `cells` is an unordered_map, so its iteration order is unspecified and
    // differs between standard libraries. These vectors are not internal -- Rule 2
    // prints one "[SC] <coord> x<v>" line per element in order, and
    // Board::any_see_each_other returns the unit of the *first* conflicting pair,
    // so an unsorted vector leaks bucket order into both the line sequence and the
    // printed unit tag. The elimination set is the same either way.
    std::pair<std::vector<Coord>, std::vector<Coord>> group_cells_by_color() const {
        std::vector<Coord> green_cells, red_cells;
        for (const auto &[coord, color] : cells) {
            if (color) { green_cells.push_back(coord); } // true = green
            else       { red_cells.push_back(coord); }   // false = red
        }
        std::sort(green_cells.begin(), green_cells.end());
        std::sort(red_cells.begin(), red_cells.end());
        return {green_cells, red_cells};
    }

    bool cell_sees_both_colors(const Cell &, const Board &) const;

    // Format: "{coord🟩,coord🟥,...}#value", cells sorted by coord so the dump is
    // deterministic across standard libraries (see the definition).
    void print(std::ostream &) const override;
};

class ColorChainTechnique : public Technique {
public:
    const char *name() const override { return "SC"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contract, NOT a leaked private: is `chain` actionable on `board`?
    // (Rule 2: a same-colored pair shares a unit; or Rule 4: some off-chain
    // candidate sees both colors.) Public so the whitebox suite judges crafted
    // chains directly, without friendship. Static, not const-member: the technique
    // is stateless and this reads only its arguments (the board it queries is
    // passed in), matching the static shape of the other hooked techniques.
    static bool test_color_chain(const Board &, const ColorChainFinding &);
};
