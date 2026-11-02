// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "cell.h"  // Cell, Value used in the test_naked_triple contract below

// Naked triple: three note cells in the same unit (row, column, or nonet) whose
// candidates together span exactly three values. Those three values must fill
// those three cells, so they can be stripped from every other cell of that unit.
// Unlike a naked pair, the cells need not agree: {1,2}, {2,3}, {1,3} is a triple,
// as is {1,2,3}, {1,2,3}, {2,3}. Each cell holds two or three of the values.
//
// NT is given-tuple shaped like NP (see docs/test-predicate-idiom.md): the find_
// loop enumerates cell triples and test_naked_triple judges each. The concrete
// NakedTripleFinding stays file-local in the .cpp, because the whitebox cases
// judge the predicate and never read a finding.
class NakedTripleTechnique : public Technique {
public:
    const char *name() const override { return "NT"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contract, NOT a leaked private: is (c1, c2, c3) a genuine, actionable
    // naked triple within `set`? Cells must be passed in ascending coord order.
    // Public static so the whitebox suite judges crafted tuples directly, without
    // friendship. Templated on the unit type; an explicit Row instantiation in the
    // .cpp forces an out-of-line symbol so the gcc -O3 cross-TU reference from the
    // test links (see docs/test-predicate-idiom.md).
    template<class Set>
    static bool test_naked_triple(const Cell &c1, const Cell &c2, const Cell &c3, const Set &set);
};
