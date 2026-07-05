// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "technique.h"

// Naked single: a note cell with exactly one remaining candidate. NS has no
// whitebox hooks, so the concrete NakedSingleFinding lives file-local in the
// .cpp; only the technique class needs to be nameable here (registry()
// constructs it directly).
class NakedSingleTechnique : public Technique {
public:
    const char *name() const override { return "NS"; }
    Tier        tier() const override { return Tier::Single; }
    bool  brace_each() const override { return false; }

    bool find(const Board &, FindingList &out) const override;
    bool apply(Board &, FindingList &mine) const override;
};
