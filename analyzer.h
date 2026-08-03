// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"
#include "board.h"
#include "technique.h"

#include <memory>
#include <vector>

class Analyzer {
public:
    Analyzer(Board &board) : mFindings(registry().size()), mBoard(board) { }

    Analyzer(Board &board, Analyzer const &other)
        : mFindings(other.mFindings)
        , mBoard(board) { }

    // The only sanctioned way to copy an Analyzer is the rebinding constructor
    // above, which re-seats mBoard onto the *new* board. The implicit copy ctor
    // would instead copy the reference, leaving the analyzer pointing at the
    // source board -- a latent dangling reference once that board dies. Delete
    // it (which also suppresses the implicit move ctor) so any accidental copy,
    // e.g. a =default'd SolverState, fails to compile rather than silently
    // aliasing the wrong board.
    Analyzer(const Analyzer &) = delete;
    Analyzer &operator=(const Analyzer &) = delete;

    void analyze();

    bool act(const bool singles_only);

    friend std::ostream& operator<< (std::ostream& outs, Analyzer const &);

    // Whitebox unit tests reach mFindings through this friend, so the
    // rebinding-ctor regression test can prove the one hand-written member copy
    // carries the findings forward. That is all it is for: the techniques
    // themselves are standalone and expose their own public seams, so nothing
    // else needs friendship. Defined in tests/unit/test_analyzer.cpp; no
    // production code depends on it.
    friend struct AnalyzerTest;

private:
    //** Notes management
    template<class Set>
    void filter_notes(Cell &, const Set &);
    void filter_notes();

private:
    //** technique registry
    // The solving techniques, constructed once and shared by every Analyzer.
    // Function-local static (defined in analyzer.cpp): built on first use, and
    // -- crucially -- never copied per state, so no technique can be dropped by
    // a missed copy in the rebinding ctor. Private: analyze(), act() and
    // operator<< all reach it via membership or friendship, and nothing else
    // needs to (AnalyzerTest does not -- see the friend declaration above).
    static const std::vector<std::unique_ptr<Technique>> &registry();

    // Per-state findings, one bucket per registry() technique, indexed parallel
    // to it. The only member carried forward across the state copy (see issue #7
    // lifecycle decision), and the only one the rebinding ctor has to name.
    // Declared before mBoard so that ctor's init list is legal under -Wreorder;
    // the rebinding-ctor regression test guards the one hand-written
    // mFindings(other.mFindings) copy.
    std::vector<FindingList> mFindings;

private:
    Board &mBoard;
};
