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
// YW is *given-tuple* shaped (docs/test-predicate-idiom.md): the pattern is the
// triple (pivot, wing1, wing2), so test_ywing below is its public predicate. It
// is the lone non-templated test_ predicate, so no explicit instantiation is
// needed (contrast test_naked_pair<Row>). YWingFinding is in this header, not
// file-local, because the whitebox cases read its fields.
struct YWingFinding : Finding {
    Value value;                    // candidate eliminated from cells seeing both wings
    Coord pivot;                    // bivalue pivot (AB)
    std::pair<Coord, Coord> wings;  // the two bivalue wings (AC, BC)

    YWingFinding(Value v, Coord p, std::pair<Coord, Coord> w)
        : value(v), pivot(p), wings(w) { }

    // find_ywing accumulates across pivots and dedups (unlike the short-circuiting
    // fish): two findings are the same Y-Wing iff value, pivot, and both wings
    // match.
    bool same(const YWingFinding &o) const {
        return value == o.value && pivot == o.pivot && wings == o.wings;
    }
    // Format: "pivotY{wing1,wing2}#value".
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
    // against what is already there. Public so the whitebox suite drives them
    // directly, without friendship -- test_ywing on crafted near-misses and
    // find_ywing on a crafted pivot (then inspecting the recorded YWingFinding).
    // Static, not const-member: the technique is stateless and both read only
    // their arguments (the board is passed in), matching the static shape of the
    // other hooked techniques.
    static bool test_ywing(const Board &, const Cell &pivot, const Cell &wing1, const Cell &wing2, std::optional<Value> &out_value);
    static bool find_ywing(const Board &, const Cell &pivot, FindingList &out);
};
