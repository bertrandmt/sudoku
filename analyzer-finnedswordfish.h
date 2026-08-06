// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value

#include <array>
#include <vector>

// Finned Swordfish: the fin rule at three base lines. A plain Swordfish needs
// three base lines whose candidates for one value fall entirely within the same
// three cross lines. A finned Swordfish allows extra candidates outside those
// three cross lines -- the fins -- provided every fin sits in a single nonet.
//
// The either/or that licenses it: if every fin is false, the base lines are
// confined to the three cover lines after all and a true Swordfish eliminates the
// value from them; if some fin is true, its nonet already holds the value. Only
// cells covered by *both* branches are safe to eliminate, i.e. cells that see
// every fin: those on a cover line, inside the fin's nonet, outside the base lines.
//
// Sashimi Swordfish (a base line holding fewer cover candidates than the fish
// needs corners, so that deleting the fin leaves it short) needs no special case:
// the rule above is stated over the fin's nonet, not over corners, so it covers
// that shape too.
//
// The relationship to FinnedXWingTechnique is the same one SwordfishTechnique has
// to XWingTechnique: the same rule at N=3 rather than N=2, written out separately.
// Unifying all four fish on one base-line-count-parameterized worker is #58; until
// then treat FX and FS as structural twins, and pair an algorithm change to one
// with the other in the same PR.
//
// Finned Swordfish is *scan-fused* (docs/test-predicate-idiom.md), like every
// other fish: the second and third base lines, the cover triple and the fin set
// are all discovered by the validating scan rather than handed in, so there is no
// separable test_ predicate. The seam is find_finned_swordfish below, and
// FinnedSwordfishFinding is in this header, not file-local, because the whitebox
// cases read its fields (mirroring FinnedXWingFinding and SwordfishFinding).
struct FinnedSwordfishFinding : Finding {
    Value value;
    // One anchor per base line: the first candidate cell in each, the same
    // recovery handle SwordfishFinding and FinnedXWingFinding use -- apply()
    // calls line_of on them to get the base lines back. An anchor may itself be a
    // fin; it identifies a line, nothing more.
    std::array<Coord, 3> anchors;
    // The fins, in board order. Unlike the plain fish, this one cannot be left to
    // apply()'s recomputation: act_on_swordfish rebuilds its cover as every cross
    // line the base lines touch, which for a finned position would drag in the
    // fin's own line and drop the mandatory fin-nonet restriction. Recording the
    // fins pins both down -- the cover is what the non-fin candidates lie on, and
    // the nonet is the one every fin shares -- so a single field replaces two that
    // could disagree with each other.
    std::vector<Coord> fins;
    bool is_row_based;  // true if rows hold the pattern, false if columns do

    FinnedSwordfishFinding(Value v, const std::array<Coord, 3> &a, std::vector<Coord> f, bool row_based)
        : value(v), anchors(a), fins(std::move(f)), is_row_based(row_based) { }
    // No same()/dedup here (unlike NP/HP/LC): find() short-circuits at the first
    // hit, so a bucket never holds two to compare.
    // Format: "{a1,a2,a3}+{f1,...}#value[^c]" -- base anchors, then the fins after
    // the '+', then the value and the unit eliminations land in (c if row-based, r
    // if column-based). FinnedXWingFinding's format at three anchors.
    void print(std::ostream &o) const override {
        o << "{" << anchors[0] << "," << anchors[1] << "," << anchors[2] << "}+{";
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

class FinnedSwordfishTechnique : public Technique {
public:
    const char *name() const override { return "FS"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Whitebox seam, NOT a leaked private: anchor the per-anchor search on a
    // chosen cell of a crafted board and inspect the finding it records --
    // reaching the rejections a happy-path solve does not isolate (fins spread
    // across two nonets, a base line made entirely of fins, a cover triple with
    // nothing to eliminate, a confined triple that is a plain Swordfish). Public
    // *static* so the whitebox suite calls it without friendship *and* without an
    // instance: the technique is stateless and this touches no instance data. Same
    // shape as XWingTechnique::find_xwing, SwordfishTechnique::find_swordfish and
    // FinnedXWingTechnique::find_finned_xwing.
    static bool find_finned_swordfish(const Board &, const Cell &, const Value &, FindingList &out);
};
