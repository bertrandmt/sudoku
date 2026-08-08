// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"

#include <cassert>
#include <memory>
#include <vector>
#include <iostream>

// Tier drives the singles_only gate in act(): Single techniques always run,
// Advanced ones only when singles_only is false.
enum class Tier { Single, Advanced };

// Type-erased finding. Concrete subtypes live either in the technique's .cpp
// (hook-free techniques) or in a shared header (hooked techniques -- see the
// test-seam contract).
struct Finding {
    virtual ~Finding() = default;
    virtual void print(std::ostream &) const = 0;
};

// shared_ptr<const> => per-state copy of mFindings is a shallow vector copy,
// no clone() ladder, no slicing (findings are immutable once found).
using FindingList = std::vector<std::shared_ptr<const Finding>>;

// Downcast a bucket entry to the concrete subtype the bucket holds. A stored
// finding's fields are only reachable through its concrete type, so every read
// of one goes through here; print() is the exception that needs no cast, being
// virtual on the base.
//
// Bucket invariant: analyze() routes reg[i]->find() into mFindings[i], so a
// bucket only ever holds the Finding subtype its own technique recorded (see
// Technique::find below), and only that technique downcasts it. Nothing but that
// discipline enforces it -- type erasure gave up the compile-time check that a
// typed member would have had -- so the assert stands in for it, turning a
// wrong-bucket wiring bug into a caught error instead of UB. It is live in the
// shipped binary: the build does not define NDEBUG.
//
// Call sites carry no comment on why the cast is sound; this is its one home.
//
// Takes the Finding, not the shared_ptr that owns it: the ownership choice
// belongs to FindingList, and a reference in and out leaves no raw pointer to
// outlive its owner.
template<class T>
const T &bucket_cast(const Finding &f) {
    assert(dynamic_cast<const T *>(&f));
    return static_cast<const T &>(f);
}

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
    // non-empty on exit -- exactly that, for every one of them: no technique
    // returns true without recording, or records without returning true. `out`
    // is this technique's own bucket and is empty on entry (each find() asserts
    // it); everything recorded must be this technique's own Finding subtype,
    // which is what licenses bucket_cast above.
    //
    // How *much* a find() enumerates is deliberately not part of the contract.
    // Most record every occurrence and apply() acts on the lot -- the greedy
    // default. Some stop at the first hit instead, which loses nothing: acting
    // on one is enough to move the state forward, and SolverState::act re-runs
    // analyze() after a successful act, so the rest are re-derived from the
    // board that has changed under them. XY-chain is one of those, and the only one
    // whose search is *ordered* rather than merely truncated: it sweeps chain
    // lengths upward so its first hit is the shortest actionable chain on the board
    // rather than whichever a depth-first walk completes first (#36).
    //
    // const in the Board: find is a pure query.
    virtual bool find(const Board &, FindingList &out) const = 0;

    // Apply this technique's findings to the board; consume (clear) them.
    virtual bool apply(Board &, FindingList &mine) const = 0;
};
