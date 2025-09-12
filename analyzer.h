// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"

#include <unordered_set>
#include <memory>

class Board;

class Analyzer {
public:
    Analyzer() { }

    Analyzer(Analyzer const &other)
        : mNakedSingles(other.mNakedSingles)
        , mHiddenSingles(other.mHiddenSingles)
        , mNakedPairs(other.mNakedPairs)
        , mLockedCandidates(other.mLockedCandidates)
        , mHiddenPairs(other.mHiddenPairs)
        , mXWings(other.mXWings)
        , mColoringGraphs(other.mColoringGraphs) { }


    using ptr = std::shared_ptr<Analyzer>;

    void board(Board &board) {
        assert(!mBoard || mBoard == &board);
        mBoard = &board;
    }
    void analyze();

    void value_dirty(const Cell &);
    void notes_dirty(const Cell &);

    bool act(const bool singles_only);

    friend std::ostream& operator<< (std::ostream& outs, Analyzer const &);

private:
    template<class Set>
    void filter_notes(Cell &, const Set &);
    template<class Set>
    void filter_notes(const Set &);
    void filter_notes();

private:
    //** naked singles
    std::vector<Coord> mNakedSingles;

    // filter
    bool test_naked_single(const Cell &) const;
    void filter_naked_singles();

    // find
    void find_naked_singles();

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

    // filter
    template<class Set>
    bool test_hidden_single(const Cell &, const Value &, const Set &, std::string &) const;
    void filter_hidden_singles();

    // find
    template<class Set>
    void find_hidden_singles(const Set &);
    void find_hidden_singles();

    // act
    bool act_on_hidden_single();

private:
    //** naked pairs
    struct NakedPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;
    };
    friend std::ostream& operator<<(std::ostream& outs, const NakedPair &);
    std::vector<NakedPair> mNakedPairs;

    // filter
    bool test_naked_pair(const Cell &, const Cell &) const;
    void filter_naked_pairs();

    // find
    template<class Set>
    void find_naked_pair(const Cell &, const Set &);
    void find_naked_pairs();

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
        bool contains(const Coord &c, const Value &v) const {
            return value == v
                && std::find(coords.begin(), coords.end(), c) != coords.end();
        }
        bool contains(const Coord &c, const Value &v, const std::string &t) const {
            return value == v
                && tag == t
                && std::find(coords.begin(), coords.end(), c) != coords.end();
        }
    };
    friend std::ostream& operator<<(std::ostream& outs, const LockedCandidates &);
    std::vector<LockedCandidates> mLockedCandidates;

    // filter
    void filter_locked_candidates();

    // find
    template<class Set1, class Set2>
    void find_locked_candidate(const Cell &, const Value &, Set1 &set_to_consider, Set2 &set_to_ignore);
    template<class Set>
    void find_locked_candidates(Set const &);
    void find_locked_candidates();

    // act
    template<class Set>
    bool act_on_locked_candidate(const LockedCandidates &, Set &);
    bool act_on_locked_candidate();

private:
    //** hidden pairs
    struct HiddenPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;
    };
    friend std::ostream& operator<<(std::ostream& outs, const HiddenPair &);
    std::vector<HiddenPair> mHiddenPairs;

    // filter
    bool test_hidden_pair(const Cell &, const Cell &, const Value &, const Value &) const;
    void filter_hidden_pairs();

    // find
    template<class Set>
    void find_hidden_pair(const Cell &, const Value &v1, const Value &v2, const Set &);
    template<class Set>
    void find_hidden_pairs(Set const &);
    void find_hidden_pairs();

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

    // filter
    template<class CandidateSet, class EliminationSet>
    bool test_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                        const EliminationSet &eset1, const EliminationSet &eset2);
    void filter_xwings();

    // find
    void find_xwing_by_row(const Cell &, const Value &);
    void find_xwing_by_column(const Cell &, const Value &);
    void find_xwing(const Cell &, const Value &);
    template<class Set>
    void find_xwing(Set const &);
    void find_xwings();

    // act
    template<class CandidateSet, class EliminationSet>
    bool act_on_xwing(const Value &value, const CandidateSet &cset1, const CandidateSet &cset2,
                                          const EliminationSet &eset, const std::string &tag);
    bool act_on_xwing();

private:
    //** simple coloring
    enum ColoringColor { kColorA, kColorB };

    struct ColoringGraph {
        Value value;
        std::unordered_map<Coord, ColoringColor> cells;  // coord -> color mapping

        bool contains(const Coord &coord) const {
            return cells.find(coord) != cells.end();
        }

        ColoringColor get_color(const Coord &coord) const {
            auto it = cells.find(coord);
            assert(it != cells.end());
            return it->second;
        }

        size_t size() const {
            return cells.size();
        }

        auto begin() const { return cells.begin(); }
        auto end() const { return cells.end(); }

        bool operator==(const ColoringGraph &other) const {
            return value == other.value && cells == other.cells;
        }
    };
    friend std::ostream& operator<<(std::ostream& outs, const ColoringGraph &);
    std::vector<ColoringGraph> mColoringGraphs;

    // filter
    bool test_coloring_graph(const ColoringGraph &graph) const;
    void filter_coloring_graphs();

    // find
    void find_coloring_graph(const Cell &cell, const Value &value);
    void find_all_coloring_graphs_for_value(const Value &value);
    template<class Set>
    void find_coloring_graphs(Set const &set);
    void find_coloring_graphs();

    // act
    bool act_on_coloring_graph();

private:
    using DirtySet = std::unordered_set<Coord>;
    DirtySet mValueDirtySet;    // this set of note cells is dirty because their value was set
    DirtySet mNotesDirtySet;    // this set of cells is dirty, because one of their notes was cleared

    Board *mBoard;
};
