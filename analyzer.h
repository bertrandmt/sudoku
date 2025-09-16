// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"
#include "board.h"

#include <unordered_set>
#include <set>
#include <unordered_map>
#include <memory>

class Analyzer {
public:
    Analyzer(Board &board) : mBoard(board) { }

    Analyzer(Board &board, Analyzer const &other)
        : mNakedSingles(other.mNakedSingles)
        , mHiddenSingles(other.mHiddenSingles)
        , mNakedPairs(other.mNakedPairs)
        , mLockedCandidates(other.mLockedCandidates)
        , mHiddenPairs(other.mHiddenPairs)
        , mXWings(other.mXWings)
        , mColorChains(other.mColorChains)
        , mYWings(other.mYWings)
        , mXYChains(other.mXYChains)
        , mBoard(board) { }

    void analyze();

    bool act(const bool singles_only);

    friend std::ostream& operator<< (std::ostream& outs, Analyzer const &);

private:
    //** Notes management
    template<class Set>
    void filter_notes(Cell &, const Set &);
    void filter_notes();

private:
    //** naked singles
    struct NakedSingle {
        Coord coord;
        Value value;
    };
    friend std::ostream& operator<<(std::ostream& outs, const NakedSingle &);
    std::vector<NakedSingle> mNakedSingles;

    // find
    bool test_naked_single(const Cell &) const;
    bool find_naked_singles();

    // act
    bool act_on_naked_single();

private:
    //** hidden singles
    struct HiddenSingle {
        Coord coord;
        Value value;
        std::string tag;
    };
    friend std::ostream& operator<<(std::ostream& outs, const HiddenSingle &);
    std::vector<HiddenSingle> mHiddenSingles;

    // find
    template<class Set>
    bool test_hidden_single(const Cell &, const Value &, const Set &, std::string &) const;
    bool find_hidden_singles();

    // act
    bool act_on_hidden_single();

private:
    //** naked pairs
    struct NakedPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;

        bool operator==(const NakedPair &other) const = default;
    };
    friend std::ostream& operator<<(std::ostream& outs, const NakedPair &);
    std::vector<NakedPair> mNakedPairs;

    // find
    template<class Set>
    bool test_naked_pair(const Cell &, const Cell &, const Set &) const;
    template<class Set>
    bool find_naked_pair(const Cell &, const Set &);
    bool find_naked_pairs();

    // act
    template<class Set>
    bool act_on_naked_pair(const NakedPair &, Set &);
    bool act_on_naked_pair();

private:
    //** locked candidates
    struct LockedCandidates {
        std::vector<Coord> coords;
        Value value;
        std::string tag;

        bool operator==(const LockedCandidates &other) const;
    };
    friend std::ostream& operator<<(std::ostream& outs, const LockedCandidates &);
    std::vector<LockedCandidates> mLockedCandidates;

    // find
    template<class Set1, class Set2>
    bool find_locked_candidate(const Cell &, const Value &, Set1 &set_to_consider, Set2 &set_to_ignore);
    bool find_locked_candidates();

    // act
    template<class Set>
    bool act_on_locked_candidate(const LockedCandidates &, Set &);
    bool act_on_locked_candidate();

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
    bool test_hidden_pair(const Cell &, const Cell &, const Value &, const Value &) const;
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
    bool test_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                        const EliminationSet &eset1, const EliminationSet &eset2);
    template<class CandidateSet, class EliminationSet>
    bool find_xwing(const Cell &, const Value &, const CandidateSet &, const EliminationSet &, const std::vector<CandidateSet> &, bool by_row);
    bool find_xwing(const Cell &, const Value &);
    bool find_xwings();

    // act
    template<class CandidateSet, class EliminationSet>
    bool act_on_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                          const EliminationSet &eset, const std::string &tag);
    bool act_on_xwing();

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
    bool test_ywing(const Cell &pivot, const Cell &wing1, const Cell &wing2, Value &out_value) const;
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
