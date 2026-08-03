// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value

// X-Wing: for one value, two base lines (both rows, or both columns) that each
// hold exactly two candidates for that value, and those candidates share the
// same two cross lines. Every other candidate for the value on those two cross
// lines can then be eliminated.
//
// X-Wing is *scan-fused* (docs/test-predicate-idiom.md), so it has no separable
// test_ predicate; the seam is find_xwing below. XWingFinding is in this header,
// not file-local, because the whitebox cases read its fields.
struct XWingFinding : Finding {
    Value value;
    Coord anchor;       // top-left corner of the X-Wing pattern
    Coord diagonal;     // bottom-right corner of the X-Wing pattern
    bool is_row_based;  // true if rows hold the pattern, false if columns do

    XWingFinding(Value v, Coord a, Coord d, bool row_based)
        : value(v), anchor(a), diagonal(d), is_row_based(row_based) { }
    // No same()/dedup here (unlike NP/HP/LC): find() short-circuits at the first
    // hit, so a bucket never holds two X-Wings to compare.
    // Format: "{anchor,diagonal}#value[^c]" (c if row-based, r if column-based).
    void print(std::ostream &o) const override {
        o << "{" << anchor << "," << diagonal << "}"
          << "#" << value << "[^" << (is_row_based ? "c" : "r") << "]";
    }
};

class XWingTechnique : public Technique {
public:
    const char *name() const override { return "XW"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Whitebox seam, NOT a leaked private: anchor the per-anchor search on a
    // chosen cell of a crafted board and inspect the finding it records --
    // exercising both orientations, the canonical-first-candidate bail, and the
    // rejection paths a happy-path solve never isolates. Public *static* so the
    // suite calls it without friendship *and* without an instance: the technique
    // is stateless and this touches no instance data.
    static bool find_xwing(const Board &, const Cell &, const Value &, FindingList &out);
};
