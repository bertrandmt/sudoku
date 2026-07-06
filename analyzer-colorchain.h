// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value, Cell used in the finding and the test_ contract below

#include <unordered_map>
#include <utility>
#include <vector>

// Simple Coloring (a.k.a. Single's Chains): for one value, build a graph of its
// candidate cells linked by bi-location (conjugate) strong links, two-color it,
// and eliminate via Rule 2 (a color repeated in a unit is false) or Rule 4 (an
// off-chain candidate that sees both colors is false).
//
// SC is a *materialized-object* technique (see docs/test-predicate-idiom.md):
// discovery (find_color_chains builds a ColorChain graph by following links) and
// validation (test_color_chain scores the eliminations that graph would make)
// are genuinely distinct steps, so it carries a separable test_ predicate --
// unlike the scan-fused X-Wing/Swordfish, whose membership emerges only from the
// validating scan. Because the graph is a first-class value, ColorChainFinding
// lives in this header (not file-local in the .cpp): the whitebox suite
// constructs one directly and hands it to test_color_chain, so both must be
// visible to tests/unit/test_analyzer.cpp.
struct ColorChainFinding : Finding {
    Value value;
    std::unordered_map<Coord, bool> cells;  // coord -> color (true=green, false=red)

    explicit ColorChainFinding(Value v) : value(v) { }

    std::pair<std::vector<Coord>, std::vector<Coord>> group_cells_by_color() const {
        std::vector<Coord> green_cells, red_cells;
        for (const auto &[coord, color] : cells) {
            if (color) { green_cells.push_back(coord); } // true = green
            else       { red_cells.push_back(coord); }   // false = red
        }
        return {green_cells, red_cells};
    }

    bool cell_sees_both_colors(const Cell &, const Board &) const;

    // Byte-for-byte the old free operator<<(ostream, Analyzer::ColorChain):
    // "{coord🟩,coord🟥,...}#value".
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
    // candidate sees both colors.) The old Analyzer::test_color_chain was reached
    // by a friend hook because techniques weren't standalone; now that
    // ColorChainTechnique is standalone this is promoted to a public static so
    // the whitebox suite judges crafted chains directly, without friendship
    // (issue #7's stated payoff). Static, not const-member: the technique is
    // stateless and this reads only its arguments (the board it queries is passed
    // in), matching the promoted-static shape of the other hooked techniques.
    static bool test_color_chain(const Board &, const ColorChainFinding &);
};
