// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"
#include "cell.h"  // Cell, Value used in the test_hidden_triple contract below

// Hidden triple: three values that, within one unit (row, column, or nonet), appear
// as candidates only in the same three cells. Those three cells must then hold those
// three values, so every other candidate can be stripped from them. Like a naked
// triple, the cells need not agree: each value sits in two or three of the cells, and
// together they cover all three. A value confined to a single cell is a hidden
// single's business, and is not reported here.
//
// HT is given-tuple shaped like HP (see docs/test-predicate-idiom.md): the find_ loop
// enumerates the cells and values and test_hidden_triple judges each. The concrete
// HiddenTripleFinding stays file-local in the .cpp, because the whitebox cases judge
// the predicate and never read a finding's fields.
class HiddenTripleTechnique : public Technique {
public:
    const char *name() const override { return "HT"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;

    // Tested contract, NOT a leaked private: is (c1, c2, c3) a genuine, actionable
    // hidden triple for values (v1, v2, v3) within `set`? Cells must be passed in
    // ascending coord order and values in ascending order. Public static so the
    // whitebox suite judges crafted tuples directly, without friendship. Templated on
    // the unit type; an explicit Row instantiation in the .cpp forces an out-of-line
    // symbol so the gcc -O3 cross-TU reference from the test links (see
    // docs/test-predicate-idiom.md).
    template<class Set>
    static bool test_hidden_triple(const Cell &c1, const Cell &c2, const Cell &c3,
                                   const Value &v1, const Value &v2, const Value &v3, const Set &set);
};
