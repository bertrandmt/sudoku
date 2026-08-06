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
// it, and exposing it would advertise a contract nothing tests. The two seams the
// cases do drive are below, and XYChainFinding is in this header because a case
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
    // walking cn..c1 from v does too). Storing one direction collapses the pair
    // into a single object, so the search cannot record two representations of one
    // deduction and no rule is needed to choose between them.
    //
    // It also fixes how the chain *prints* -- the [fXY] trace, the analyzer dump
    // and the "{front:..:back}" each [XY] line carries. Direction was free to vary
    // without changing the conclusion, and that is exactly what made it reach
    // stdout unpinned by anything (#53).
    //
    // The retention rule's tie-break would pick the same direction on its own
    // wherever both directions are offered, which is everywhere find() searches:
    // for a reversed pair, "lexicographically smaller chain" and "front < back"
    // are the same test. Normalizing here is what makes the invariant hold of a
    // finding rather than of a search that happened to see both.
    XYChainFinding(Value v, std::vector<Coord> c, std::set<Coord> e)
        : value(v), chain(std::move(c)), eliminations(std::move(e)) {
        if (chain.size() > 1 && chain.back() < chain.front())
            std::reverse(chain.begin(), chain.end());
    }

    // Does this finding's effect cover `other`'s entirely? Same eliminated value,
    // and every cell `other` clears is one this clears too. The retention rule
    // (see record_if_maximal) keeps the inclusion-maximal effects and drops what
    // they already cover.
    //
    // Note this is reflexive: a finding subsumes itself. Callers compare distinct
    // findings, and the identical-set case is settled by tighter_than instead.
    bool subsumes(const XYChainFinding &other) const;

    // Tie-break between two findings with the *same* elimination set: prefer the
    // shorter chain -- the tighter justification for an identical conclusion --
    // and, at equal length, the lexicographically smaller chain. That last term
    // has no meaning to prefer; its job is to make the choice a total function of
    // the two chains rather than of the order the search offered them in.
    bool tighter_than(const XYChainFinding &other) const;

    // Canonical emission order: by eliminated value, then by elimination set,
    // then by chain. A total order on distinct findings, so sorting by it leaves
    // nothing for the search's traversal order to decide. The chain term is
    // redundant against the retention rule, which already leaves at most one
    // finding per (value, elimination set) -- it is there so that determinism
    // rests on this comparator alone and not on an invariant established
    // elsewhere.
    bool precedes(const XYChainFinding &other) const;

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
    //
    // `out` comes back in canonical order, as it does from find() -- ordering is
    // record_if_maximal's job, not a pass at the end. What this does not do is emit
    // the [fXY] trace, which reports a completed search over the whole board and
    // this is one anchor of it.
    static bool find_xychain(const Board &, const Cell &, const Value &, FindingList &out);

    // Tested contract, NOT a leaked private: `out` retains the inclusion-maximal
    // elimination effects offered so far, one finding per distinct effect, in
    // canonical order (see precedes). An offer is dropped if a retained finding
    // already covers it; otherwise it is inserted at its canonical position and
    // anything it covers is removed. Among offers with an identical elimination
    // set, the tighter one (see tighter_than) is kept. Returns whether `candidate`
    // was retained.
    //
    // Both halves of that -- which findings are held and what order they are held
    // in -- are independent of the order offers arrive in, which is what licenses
    // deleting the traversal sort extend_chain used to carry: for sets A subset of
    // B either arrival order ends at {B}, non-nesting effects are both kept either
    // way, and identical sets are settled by a total order on the chains rather
    // than by arrival.
    //
    // Exposed because the retention rule is the one XY-chain invariant a crafted
    // board cannot isolate: it takes several competing chains to exercise, and
    // which chains a board yields is not controllable. The whitebox case offers
    // them directly instead.
    static bool record_if_maximal(FindingList &out, const XYChainFinding &candidate);
};
