// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value

#include <array>

// Swordfish: X-Wing widened from two base lines to three. For one value, three
// base lines (all rows, or all columns) that each hold two or three candidates
// for that value, whose cross lines union to exactly three. Every other
// candidate for the value on those three cross lines can then be eliminated.
//
// Swordfish is *scan-fused* (docs/test-predicate-idiom.md), like its sibling
// X-Wing: the second and third base lines are discovered by the validating scan
// rather than handed in, so there is no separable test_ predicate. The seam is
// find_swordfish below, and SwordfishFinding is in this header, not file-local,
// because the whitebox cases read its fields (mirroring XWingFinding).
struct SwordfishFinding : Finding {
    Value value;
    // One anchor per base line: the first candidate cell encountered in each.
    // Not "corners" -- that is X-Wing's vocabulary, where the two recorded cells
    // really are opposite corners of the rectangle. Here the anchors are how
    // apply() recovers the pattern: it calls line_of<CandidateSet> on each one to
    // reconstruct the three base lines. Fixed-size, so apply()'s [0]/[1]/[2] are
    // safe by construction rather than by an invariant a reader has to trust.
    std::array<Coord, 3> anchors;
    bool is_row_based;  // true if rows hold the pattern, false if columns do

    SwordfishFinding(Value v, const std::array<Coord, 3> &a, bool row_based)
        : value(v), anchors(a), is_row_based(row_based) { }
    // No same()/dedup here (unlike NP/HP/LC): find() short-circuits at the first
    // hit, so a bucket never holds two Swordfish to compare.
    // Format: "{a1,a2,a3}#value[^c]" (c if row-based, r if column-based).
    void print(std::ostream &o) const override {
        o << "{";
        bool is_first = true;
        for (const auto &anchor : anchors) {
            if (!is_first) o << ",";
            is_first = false;
            o << anchor;
        }
        o << "}"
          << "#" << value << "[^" << (is_row_based ? "c" : "r") << "]";
    }
};

class SwordfishTechnique : public Technique {
public:
    const char *name() const override { return "SF"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Whitebox seam, NOT a leaked private: anchor the per-anchor search on a
    // chosen cell of a crafted board and inspect the finding it records --
    // reaching the no-eliminations rejection that a happy-path solve does not
    // isolate, and pinning the tight-cover-line rule on a position built for it.
    // (Both orientations are already covered black-box: P_hard
    // fires a row-based Swordfish, P_sf a column-based one.) Public *static* so
    // the whitebox suite calls it without friendship *and* without an instance:
    // the technique is stateless and this touches no instance data. Same shape as
    // XWingTechnique::find_xwing.
    static bool find_swordfish(const Board &, const Cell &, const Value &, FindingList &out);
};
