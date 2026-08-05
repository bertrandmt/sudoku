// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value

#include <array>
#include <vector>

// Finned X-Wing: X-Wing with its confinement requirement relaxed. A plain
// X-Wing needs two base lines whose candidates for one value fall entirely
// within the same two cross lines. A finned X-Wing allows extra candidates
// outside those two cross lines -- the fins -- provided every fin sits in a
// single box.
//
// The either/or that licenses it: if every fin is false, the base lines are
// confined to the two cover lines after all and a true X-Wing eliminates the
// value from them; if some fin is true, its box already holds the value. Only
// cells covered by *both* branches are safe to eliminate, i.e. cells that see
// every fin: those on a cover line, inside the fin box, outside the base lines.
//
// Sashimi X-Wing (a base line holding only one cover candidate, so that deleting
// the fin leaves the fish a corner short) needs no special case: the rule above
// is stated over the fin box, not over corners, so it covers that shape too.
//
// Finned X-Wing is *scan-fused* (docs/test-predicate-idiom.md), like the plain
// fish it extends: the second base line, the cover pair and the fin set are all
// discovered by the validating scan rather than handed in, so there is no
// separable test_ predicate. The seam is find_finned_xwing below, and
// FinnedXWingFinding is in this header, not file-local, because the whitebox
// cases read its fields (mirroring XWingFinding and SwordfishFinding).
struct FinnedXWingFinding : Finding {
    Value value;
    // One anchor per base line: the first candidate cell in each, the same
    // recovery handle SwordfishFinding uses -- apply() calls line_of on them to
    // get the base lines back. An anchor may itself be a fin; it identifies a
    // line, nothing more.
    std::array<Coord, 2> anchors;
    // The fins, in board order. Unlike the plain fish, this one cannot be left
    // to apply()'s recomputation: act_on_xwing rebuilds its cover as every cross
    // line the base lines touch, which for a finned position would drag in the
    // fin's own line and drop the mandatory fin-box restriction. Recording the
    // fins pins both down -- the cover is what the non-fin candidates lie on,
    // and the box is the one every fin shares -- so a single field replaces two
    // that could disagree with each other.
    std::vector<Coord> fins;
    bool is_row_based;  // true if rows hold the pattern, false if columns do

    FinnedXWingFinding(Value v, const std::array<Coord, 2> &a, std::vector<Coord> f, bool row_based)
        : value(v), anchors(a), fins(std::move(f)), is_row_based(row_based) { }
    // No same()/dedup here (unlike NP/HP/LC): find() short-circuits at the first
    // hit, so a bucket never holds two to compare.
    // Format: "{a1,a2}+{f1,...}#value[^c]" -- base anchors, then the fins after
    // the '+', then the value and the unit eliminations land in (c if row-based,
    // r if column-based). Extends XWingFinding's format with the one thing a
    // finned position adds.
    void print(std::ostream &o) const override {
        o << "{" << anchors[0] << "," << anchors[1] << "}+{";
        bool is_first = true;
        for (const auto &fin : fins) {
            if (!is_first) o << ",";
            is_first = false;
            o << fin;
        }
        o << "}"
          << "#" << value << "[^" << (is_row_based ? "c" : "r") << "]";
    }
};

class FinnedXWingTechnique : public Technique {
public:
    const char *name() const override { return "FX"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Whitebox seam, NOT a leaked private: anchor the per-anchor search on a
    // chosen cell of a crafted board and inspect the finding it records --
    // reaching the rejections a happy-path solve does not isolate (fins spread
    // across two boxes, a base line made entirely of fins, a cover pair with
    // nothing to eliminate). Public *static* so the whitebox suite calls it
    // without friendship *and* without an instance: the technique is stateless
    // and this touches no instance data. Same shape as XWingTechnique::find_xwing
    // and SwordfishTechnique::find_swordfish.
    static bool find_finned_xwing(const Board &, const Cell &, const Value &, FindingList &out);
};
