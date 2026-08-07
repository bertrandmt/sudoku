// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

// XY-Chain: a sequence of bi-value cells in which each consecutive pair shares
// exactly one candidate. Follow the chain from one end's candidate X -- whichever
// way that end resolves, one of the two ends must hold X -- so any off-chain cell
// seeing *both* ends can drop X.
//
// XY-chain is *materialized-object* shaped (docs/test-predicate-idiom.md), like
// simple coloring. Its scoring predicate stays file-local: no whitebox case calls
// it, and exposing it would advertise a contract nothing tests. The one seam the
// cases do drive is below, and XYChainFinding is in this header because a case
// reads the recorded chain's fields.
struct XYChainFinding : Finding {
    Value value;                     // candidate to eliminate from cells seeing both chain ends
    std::vector<Coord> chain;        // sequence of XY-cells forming the chain
    std::set<Coord> eliminations;    // the off-chain cells that lose `value`

    // The chain is stored in *canonical direction*: front() < back(), reversing
    // the caller's sequence if need be. A chain and its reverse are the same
    // deduction -- same cells, same value, same eliminations -- and reversing one
    // yields a chain that still links up and still closes (each cell is bi-value,
    // so `other_value` is an involution: if walking c1..cn from v exits at v, then
    // walking cn..c1 from v does too).
    //
    // What this buys is a canonical *representation*, not determinism. find() tries
    // every bi-value cell as an anchor, so it meets a chain and its reverse from two
    // different anchors and acts on whichever it reaches first; normalizing means both
    // produce the same object, so the printed chain no longer encodes which anchor
    // that was. That anchor order was never in question -- `board.cells()` is a
    // vector, so it is the same on every standard library -- and direction reaches
    // nothing else: apply() replays a coord-ordered set, so neither the cells cleared
    // nor the order of the [XY] lines depends on it. Only "{front:..:back}" does.
    //
    // The unspecified-order problem #53 filed is a different one and is pinned
    // elsewhere: see the sort in extend_chain.
    XYChainFinding(Value v, std::vector<Coord> c, std::set<Coord> e)
        : value(v), chain(std::move(c)), eliminations(std::move(e)) {
        if (chain.size() > 1 && chain.back() < chain.front())
            std::reverse(chain.begin(), chain.end());
    }

    // Format: "{c1:c2:...}#value" followed by "x" and the elimination count.
    void print(std::ostream &) const override;
};

class XYChainTechnique : public Technique {
public:
    const char *name() const override { return "XY"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Whitebox seam, NOT a leaked private: anchor the chain search on one cell and
    // one of its two candidates, so a case can drive detection on a crafted
    // three-cell chain instead of hunting for a board whose whole cascade falls
    // through to XY. Static because the technique holds no state and this entry
    // touches none, matching the other seams.
    //
    // `max_len` bounds the chain length this call will build, and is the reason the
    // seam carries a parameter the other techniques' do not: find() sweeps it
    // upwards (see there), and a case that wants one specific chain has to say how
    // long to look. Records at most one finding and returns whether it did; a
    // non-empty `out` on entry is treated as "already found" and short-circuits,
    // which is how find() stops its sweep.
    static bool find_xychain(const Board &, const Cell &, const Value &, size_t max_len, FindingList &out);
};
