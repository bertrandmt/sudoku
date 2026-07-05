// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"

// Hidden single: a candidate value with exactly one possible cell in some unit
// (row, column, or nonet). Like Naked Singles, HS has no whitebox hooks, so the
// concrete HiddenSingleFinding lives file-local in the .cpp; only the technique
// class needs to be nameable here (registry() constructs it directly).
class HiddenSingleTechnique : public Technique {
public:
    const char *name() const override { return "HS"; }
    Tier        tier() const override { return Tier::Single; }
    bool  brace_each() const override { return false; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;
};
