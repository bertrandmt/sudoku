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
    // Abstraction tag, e.g. "NS", "HS". Named name() rather than tag() on
    // purpose: board.h has a free tag(Unit), and a member named tag() would
    // shadow it inside every technique's find/apply/dump that prints a Unit,
    // forcing a ::tag qualifier at each such call. name() sidesteps that.
    virtual const char *name() const = 0;
    virtual Tier        tier() const = 0;
    virtual bool        brace_each() const = 0;  // print_section wrap flag

    // Search the board, record findings into `out`, and return whether `out` is
    // non-empty on exit -- exactly that, for all ten: no technique returns true
    // without recording, or records without returning true. `out` is this
    // technique's own bucket and is empty on entry (each find() asserts it);
    // everything recorded must be this technique's own Finding subtype, which is
    // what licenses the downcast in apply().
    //
    // How *much* a find() enumerates is deliberately not part of the contract.
    // Most record every occurrence and apply() acts on the lot -- the greedy
    // default. Some stop at the first hit instead, which loses nothing: acting
    // on one is enough to move the state forward, and SolverState::act re-runs
    // analyze() after a successful act, so the rest are re-derived from the
    // board that has changed under them. XY-chain is the one that searches
    // exhaustively and then keeps only its best finding -- a real deviation from
    // the greedy default, under review in issue #36.
    //
    // const in the Board: find is a pure query.
    virtual bool find(const Board &, FindingList &out) const = 0;

    // Apply this technique's findings to the board; consume (clear) them.
    virtual bool apply(Board &, FindingList &mine) const = 0;
};
