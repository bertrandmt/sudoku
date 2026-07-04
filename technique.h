// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"

#include <memory>
#include <vector>
#include <iostream>

// Tier drives the singles_only gate in act(): Single techniques always run,
// Advanced ones only when singles_only is false.
enum class Tier { Single, Advanced };

// Type-erased finding. Concrete subtypes live either in the technique's .cpp
// (hook-free techniques) or in a shared header (hooked techniques -- see the
// test-seam contract). Only the owning technique downcasts its own bucket.
struct Finding {
    virtual ~Finding() = default;
    virtual void print(std::ostream &) const = 0;
};

// shared_ptr<const> => per-state copy of mFindings is a shallow vector copy,
// no clone() ladder, no slicing (findings are immutable once found).
using FindingList = std::vector<std::shared_ptr<const Finding>>;

class Technique {
public:
    virtual ~Technique() = default;
    virtual const char *tag()  const = 0;   // "NS", "HS", ...
    virtual Tier        tier() const = 0;
    virtual bool        brace_each() const = 0;  // print_section wrap flag

    // Find every occurrence on the board; append to `out`. Return whether any
    // were found. const in the Board: find is a pure query.
    virtual bool find(const Board &, FindingList &out) const = 0;

    // Apply this technique's findings to the board; consume (clear) them.
    virtual bool apply(Board &, FindingList &mine) const = 0;
};
