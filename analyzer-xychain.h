// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "coord.h"
#include "cell.h"  // Value

#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

// XY-Chain: a sequence of bi-value cells in which each consecutive pair shares
// exactly one candidate. Follow the chain from one end's candidate X -- whichever
// way that end resolves, one of the two ends must hold X -- so any off-chain cell
// seeing *both* ends can drop X.
//
// XY-chain is *materialized-object* shaped (docs/test-predicate-idiom.md), like
// simple coloring. Its scoring predicate stays file-local: no whitebox case calls
// it, and exposing it would advertise a contract nothing tests. The two seams the
// cases do drive are below, and XYChainFinding is in this header because a case
// reads the recorded chain's fields.
struct XYChainFinding : Finding {
    Value value;                // candidate to eliminate from cells seeing both chain ends
    std::vector<Coord> chain;   // sequence of XY-cells forming the chain
    size_t num_elim;            // how many off-chain cells that elimination clears

    XYChainFinding(Value v, std::vector<Coord> c, size_t n)
        : value(v), chain(std::move(c)), num_elim(n) { }

    // Two XY-chains are equivalent if they have the same elimination value
    // and the same endpoints, regardless of the internal path
    bool operator==(const XYChainFinding &other) const {
        if (value != other.value) return false;
        return (chain.front() == other.chain.front() && chain.back() == other.chain.back()) ||
               (chain.front() == other.chain.back() && chain.back() == other.chain.front());
    }

    // Desirability order, the rule record_if_best applies: more eliminations
    // first, ties broken toward the shorter chain. "Less" means "better" here --
    // it reads backwards on purpose, because this ordering's one job is to put
    // the chain worth acting on first.
    bool operator<(const XYChainFinding &other) const {
        return (num_elim >  other.num_elim) ||
               (num_elim == other.num_elim && chain.size() < other.chain.size());
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

    // Whitebox seam, NOT a leaked private: anchor the chain search on one cell
    // and one of its two candidates, so a case can drive detection on a crafted
    // three-cell chain instead of hunting for a board whose whole cascade falls
    // through to XY. Static because the technique holds no state and this entry
    // touches none, matching the other seams.
    static bool find_xychain(const Board &, const Cell &, const Value &, FindingList &out);

    // Tested contract, NOT a leaked private: `out` retains at most one chain, the
    // most desirable one offered so far (see XYChainFinding::operator<), and
    // `candidate` replaces it only by being strictly better. Returns whether it
    // did -- which is also this technique's "found something new" signal, so the
    // verbose [fXY] line is emitted here. Exposed because the selection rule is
    // the one XY-chain invariant a crafted board cannot isolate: it takes several
    // competing chains to exercise, and which chains a board yields is not
    // controllable. The whitebox case offers them directly instead.
    //
    // Trimming to one is what the solver does today, not settled design; see #36.
    static bool record_if_best(FindingList &out, const XYChainFinding &candidate);
};
