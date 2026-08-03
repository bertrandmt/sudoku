// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "cell.h"  // Cell, Value used in the test_hidden_pair contract below

// Hidden pair: two values that, within one unit (row, column, or nonet), appear
// as candidates in exactly the same two cells and nowhere else in that unit.
// Those two cells are then locked to that pair, so every other candidate can be
// stripped from them.
//
// Like Naked Pairs, HP is a *hooked* technique: its validation predicate is
// exercised directly by the whitebox suite. The concrete HiddenPairFinding still
// lives file-local in the .cpp (the hook tests the predicate, not the finding),
// but test_hidden_pair is promoted here to a tested public contract -- see below.
class HiddenPairTechnique : public Technique {
public:
    const char *name() const override { return "HP"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contract, NOT a leaked private: is (c1, c2) a genuine, actionable
    // hidden pair for values (v1, v2) within `set`? Public static so the whitebox
    // suite judges crafted tuples directly, without friendship. It requires v1 < v2
    // and c1 before c2: find_ enumerates in that order, but a direct test caller
    // can pass either way, so the predicate guards the precondition itself.
    // Templated on the unit type; an explicit Row instantiation in the .cpp forces
    // an out-of-line symbol so the gcc -O3 cross-TU reference from the test links
    // (see docs/test-predicate-idiom.md).
    template<class Set>
    static bool test_hidden_pair(const Cell &c1, const Cell &c2, const Value &v1, const Value &v2, const Set &set);
};
