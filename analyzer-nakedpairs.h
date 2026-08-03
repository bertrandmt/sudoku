// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "cell.h"  // Cell, Value used in the test_naked_pair contract below

// Naked pair: two note cells in the same unit (row, column, or nonet) sharing
// the exact same bivalue candidate set; those two values can then be stripped
// from every other cell of that unit.
//
// NP is a *hooked* technique: unlike Naked/Hidden Singles, its validation
// predicate is exercised directly by the whitebox suite. The concrete
// NakedPairFinding stays file-local in the .cpp (the hook tests the predicate,
// not the finding); test_naked_pair is the tested public contract -- see below.
class NakedPairTechnique : public Technique {
public:
    const char *name() const override { return "NP"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contract, NOT a leaked private: is (c1, c2) a genuine, actionable
    // naked pair within `set`? Public static so the whitebox suite judges crafted
    // tuples directly, without friendship. Templated on the unit type; an explicit
    // Row instantiation in the .cpp forces an out-of-line symbol so the gcc -O3
    // cross-TU reference from the test links (see docs/test-predicate-idiom.md).
    template<class Set>
    static bool test_naked_pair(const Cell &c1, const Cell &c2, const Set &set);
};
