// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value, Cell used in the finding and the test_ contract below

#include <optional>
#include <utility>

// Y-Wing: a bivalue pivot cell (candidates AB) that sees two bivalue wings, one
// carrying AC and one carrying BC. Any cell that sees both wings can then have
// candidate C eliminated. (https://www.sudokuwiki.org/Y_Wing_Strategy)
//
// YW is *given-tuple* shaped (see docs/test-predicate-idiom.md): the pattern's
// identity is the triple (pivot, wing1, wing2), which find enumerates and a pure
// test_ywing predicate judges -- so, like naked/hidden pair, that predicate
// promotes to a public static. It is the lone *non-templated* test_ predicate,
// so no explicit-instantiation tax applies (contrast test_naked_pair<Row>).
//
// Unlike the file-local NakedPairFinding, YWingFinding lives in this header: the
// whitebox suite drives find_ywing on a crafted pivot and inspects the recorded
// finding's value (like the scan-fused XWingFinding), so the type must be visible
// to tests/unit/test_analyzer.cpp. The finding is exposed exactly when the test
// must read its fields, not merely because the technique is hooked.
struct YWingFinding : Finding {
    Value value;                    // candidate eliminated from cells seeing both wings
    Coord pivot;                    // bivalue pivot (AB)
    std::pair<Coord, Coord> wings;  // the two bivalue wings (AC, BC)

    YWingFinding(Value v, Coord p, std::pair<Coord, Coord> w)
        : value(v), pivot(p), wings(w) { }

    // find_ywing accumulates across pivots and dedups (unlike the short-circuiting
    // fish): two findings are the same Y-Wing iff value, pivot, and both wings
    // match -- byte-for-byte the old Analyzer::YWing's defaulted operator==.
    bool same(const YWingFinding &o) const {
        return value == o.value && pivot == o.pivot && wings == o.wings;
    }
    // Byte-for-byte the old free operator<<(ostream, Analyzer::YWing):
    // "pivotY{wing1,wing2}#value".
    void print(std::ostream &o) const override {
        o << pivot << "Y{" << wings.first << "," << wings.second << "}#" << value;
    }
};

class YWingTechnique : public Technique {
public:
    const char *name() const override { return "YW"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contracts, NOT leaked privates. test_ywing: is (pivot, wing1, wing2)
    // a genuine, actionable Y-Wing (and if so, what value does it eliminate)?
    // find_ywing: record every Y-Wing anchored on `pivot` into `out`, deduping
    // against what is already there. The old Analyzer members were reached by a
    // friend hook because techniques weren't standalone; now that YWingTechnique
    // is standalone both promote to public statics (issue #7's stated payoff) so
    // the whitebox suite drives them directly -- test_ywing on crafted near-misses
    // and find_ywing on a crafted pivot (then inspecting the recorded
    // YWingFinding) -- without friendship. Static, not const-member: the technique
    // is stateless and both read only their arguments (the board is passed in),
    // matching the promoted-static shape of the other hooked techniques.
    static bool test_ywing(const Board &, const Cell &pivot, const Cell &wing1, const Cell &wing2, std::optional<Value> &out_value);
    static bool find_ywing(const Board &, const Cell &pivot, FindingList &out);
};
