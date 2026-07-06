// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"

// Locked candidates: a value whose candidate cells in one unit (row, column, or
// nonet) all fall inside a second, overlapping unit. The value is then locked to
// that overlap and can be eliminated from the rest of the second unit.
// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
//
// Like Naked/Hidden Singles, LC has no whitebox hooks, so the concrete
// LockedCandidatesFinding lives file-local in the .cpp; only the technique class
// needs to be nameable here (registry() constructs it directly).
class LockedCandidatesTechnique : public Technique {
public:
    const char *name() const override { return "LC"; }
    Tier        tier() const override { return Tier::Advanced; }
    bool  brace_each() const override { return true; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;
};
