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
// X-Wing is *scan-fused* (see docs/test-predicate-idiom.md): the pattern's
// membership emerges only from the validating scan, so there is no separable
// test_ predicate to promote -- the whitebox suite drives the per-anchor search
// directly and inspects the recorded finding. That is why XWingFinding lives in
// this header rather than file-local in the .cpp (unlike NS/HS/NP/LC/HP, whose
// findings the tests never name): the scan-fused seam needs to read the
// finding's fields, so the type must be visible to tests/unit/test_analyzer.cpp.
struct XWingFinding : Finding {
    Value value;
    Coord anchor;       // top-left corner of the X-Wing pattern
    Coord diagonal;     // bottom-right corner of the X-Wing pattern
    bool is_row_based;  // true if rows hold the pattern, false if columns do

    XWingFinding(Value v, Coord a, Coord d, bool row_based)
        : value(v), anchor(a), diagonal(d), is_row_based(row_based) { }
    // No same()/dedup here (unlike NP/HP/LC): find() short-circuits at the first
    // hit, so a bucket never holds two X-Wings to compare.
    // Byte-for-byte the old free operator<<(ostream, Analyzer::XWing):
    // "{anchor,diagonal}#value[^c]" (c if row-based, r if column-based).
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

    // Whitebox seam, NOT a leaked private: drive the per-anchor search on a
    // crafted board and inspect the finding it records. X-Wing is scan-fused, so
    // there is no test_ predicate to call directly (see docs/test-predicate-idiom.md);
    // the tests instead anchor find_xwing on a chosen cell -- exercising both
    // orientations, the canonical-first-candidate bail, and the rejection paths a
    // happy-path solve never isolates. Public *static* so the whitebox suite calls
    // it without friendship *and* without an instance (issue #7's stated payoff),
    // matching the promoted-static shape of the given-tuple predicates: the
    // technique is stateless and this touches no instance data.
    static bool find_xwing(const Board &, const Cell &, const Value &, FindingList &out);
};
