// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "cell.h"  // Cell, Value used in the test_naked_pair contract below

// Naked pair: two note cells in the same unit (row, column, or nonet) sharing
// the exact same bivalue candidate set; those two values can then be stripped
// from every other cell of that unit.
//
// NP is the first *hooked* technique to port (issue #7): unlike Naked/Hidden
// Singles, its validation predicate is exercised directly by the whitebox suite.
// The concrete NakedPairFinding still lives file-local in the .cpp (the hook
// tests the predicate, not the finding), but test_naked_pair is promoted here to
// a tested public contract -- see below.
class NakedPairTechnique : public Technique {
public:
    const char *name() const override { return "NP"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contract, NOT a leaked private: is (c1, c2) a genuine, actionable
    // naked pair within `set`? The old Analyzer::test_naked_pair was reached by a
    // friend hook because techniques weren't standalone; now that NakedPairTechnique
    // is standalone this is promoted to a public static so the whitebox suite judges
    // crafted tuples directly, without friendship (issue #7's stated payoff).
    // Templated on the unit type; an explicit Row instantiation in the .cpp forces
    // an out-of-line symbol so the gcc -O3 cross-TU reference from the test links
    // (see docs/test-predicate-idiom.md).
    template<class Set>
    static bool test_naked_pair(const Cell &c1, const Cell &c2, const Set &set);
};
