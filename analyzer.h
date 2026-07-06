// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"
#include "board.h"
#include "technique.h"

#include <unordered_set>
#include <set>
#include <unordered_map>
#include <memory>
#include <optional>

class Analyzer {
public:
    Analyzer(Board &board) : mFindings(registry().size()), mBoard(board) { }

    Analyzer(Board &board, Analyzer const &other)
        : mFindings(other.mFindings)
        , mHiddenPairs(other.mHiddenPairs)
        , mXWings(other.mXWings)
        , mSwordfish(other.mSwordfish)
        , mColorChains(other.mColorChains)
        , mYWings(other.mYWings)
        , mXYChains(other.mXYChains)
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

    // Whitebox unit tests reach the private find_*/act_* members and the result
    // vectors through this friend, so individual techniques can be exercised on
    // hand-built positions that a full REPL solve cannot easily reproduce.
    // Defined in tests/unit/test_analyzer.cpp; no production code depends on it.
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
    // -- crucially -- never copied per state, so forgetting to copy a technique
    // in the rebinding ctor is no longer possible (issue #7). Private: analyze()/
    // act()/operator<</AnalyzerTest all reach it via membership or friendship.
    static const std::vector<std::unique_ptr<Technique>> &registry();

    // Per-state findings, one bucket per registry() technique, indexed parallel
    // to it. The sole member still carried forward across the state copy (see
    // issue #7 lifecycle decision). Declared before every other technique member
    // and before mBoard so the rebinding ctor's init list is legal under
    // -Wreorder; the rebinding-ctor regression test guards the one hand-written
    // mFindings(other.mFindings) copy.
    std::vector<FindingList> mFindings;

private:
    //** hidden pairs
    struct HiddenPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;

        bool operator==(const HiddenPair &other) const = default;
    };
    friend std::ostream& operator<<(std::ostream& outs, const HiddenPair &);
    std::vector<HiddenPair> mHiddenPairs;

    // find
    template<class Set>
    bool test_hidden_pair(const Cell &, const Cell &, const Value &, const Value &, const Set &) const;
    template<class Set>
    bool find_hidden_pair(const Cell &, const Value &v1, const Value &v2, const Set &);
    bool find_hidden_pairs();

    // act
    bool act_on_hidden_pair(Cell &, const HiddenPair &);
    bool act_on_hidden_pair();

private:
    //** x-wing
    struct XWing {
        Value value;
        Coord anchor;       // top-left corner of the XWing pattern
        Coord diagonal;     // bottom-right corner of the XWing pattern
        bool is_row_based;  // true if rows contain the pattern, false if columns contain the pattern

        bool operator==(const XWing &other) const = default;
    };
    friend std::ostream& operator<<(std::ostream& outs, const XWing &);
    std::vector<XWing> mXWings;

    // find
    template<class CandidateSet, class EliminationSet>
    bool find_xwing(const Cell &, const Value &, const CandidateSet &, const EliminationSet &, const std::vector<CandidateSet> &, bool by_row);
    bool find_xwing(const Cell &, const Value &);
    bool find_xwings();

    // act
    template<class CandidateSet, class EliminationSet>
    bool act_on_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                          const EliminationSet &eset, Unit unit);
    template<class EliminationSet>
    bool act_on_xwing(const XWing &entry);
    bool act_on_xwing();

private:
    //** swordfish
    struct Swordfish {
        Value value;
        std::vector<Coord> anchors;  // three corners defining the Swordfish pattern
        bool is_row_based;           // true if rows contain the pattern, false if columns contain the pattern

        bool operator==(const Swordfish &other) const = default;
    };
    friend std::ostream& operator<<(std::ostream& outs, const Swordfish &);
    std::vector<Swordfish> mSwordfish;

    // find
    template<class CandidateSet, class EliminationSet>
    bool find_swordfish(const Cell &, const Value &, const CandidateSet &, const EliminationSet &, const std::vector<CandidateSet> &, bool by_row);
    bool find_swordfish(const Cell &, const Value &);
    bool find_swordfish();

    // act
    template<class CandidateSet, class EliminationSet>
    bool act_on_swordfish(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2, const CandidateSet &cset3,
                                              const EliminationSet &eset, Unit unit);
    template<class EliminationSet>
    bool act_on_swordfish(const Swordfish &entry);
    bool act_on_swordfish();

private:
    //** simple coloring
    struct ColorChain {
        Value value;
        std::unordered_map<Coord, bool> cells;  // coord -> color mapping (true=green, false=red)

        std::pair<std::vector<Coord>, std::vector<Coord>> group_cells_by_color() const {
            std::vector<Coord> green_cells, red_cells;
            for (const auto &[coord, color] : cells) {
                if (color) { green_cells.push_back(coord); } // true = green
                else       { red_cells.push_back(coord); }   // false = red
            }
            return {green_cells, red_cells};
        }

        bool cell_sees_both_colors(const Cell &, const Board &) const;
    };
    friend std::ostream& operator<<(std::ostream& outs, const ColorChain &);
    std::vector<ColorChain> mColorChains;

    // find
    bool test_color_chain(const ColorChain &chain) const;
    bool find_color_chains(const Value &value);
    bool find_color_chains();

    // act
    bool act_on_color_chain();

private:
    //** y-wing
    struct YWing {
        Value value;                   // candidate to eliminate from cells seeing both wings
        Coord pivot;                   // pivot cell with 2 candidates (AB)
        std::pair<Coord, Coord> wings; // wing cell sharing candidate A with pivot

        bool operator==(const YWing &other) const = default;
    };
    friend std::ostream& operator<<(std::ostream& outs, const YWing &);
    std::vector<YWing> mYWings;

    // find
    bool test_ywing(const Cell &pivot, const Cell &wing1, const Cell &wing2, std::optional<Value> &out_value) const;
    bool find_ywing(const Cell &pivot);
    bool find_ywings();

    // act
    template<class Set>
    bool act_on_ywing(const YWing &entry, const Set &set);
    bool act_on_ywing();

private:
    //** xy-chain
    struct XYChain {
        Value value;                   // candidate to eliminate from cells seeing both chain ends
        std::vector<Coord> chain;      // sequence of XY-cells forming the chain
        size_t num_elim;

        bool operator==(const XYChain &other) const {
            // Two XY-chains are equivalent if they have the same elimination value
            // and the same endpoints, regardless of the internal path
            if (value != other.value) return false;
            return (chain.front() == other.chain.front() && chain.back() == other.chain.back()) ||
                   (chain.front() == other.chain.back() && chain.back() == other.chain.front());
        }

        bool operator<(const XYChain &other) const {
            return (num_elim >  other.num_elim) ||
                   (num_elim == other.num_elim && chain.size() < other.chain.size());
        }
    };
    friend std::ostream& operator<<(std::ostream& outs, const XYChain &);
    std::set<XYChain> mXYChains;

    // find
    size_t test_xychain(const Value &value, const std::vector<Coord> &chain) const;
    bool find_xychain(const Cell &, const Value &);
    bool find_xychains();

    // act
    template<class Set>
    bool act_on_xychain(const XYChain &entry, const Set &chain_front_set);
    bool act_on_xychain();

private:
    Board &mBoard;
};
