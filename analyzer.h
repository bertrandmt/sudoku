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
        , mHiddenPairs(other.mHiddenPairs) { }


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
    void filter_notes_for_set(Cell &, const Set &);
    template<class Set>
    void filter_notes(const Set &);
    void filter_notes();

    // naked singles
    std::vector<Coord> mNakedSingles;

    bool test_naked_single(const Cell &) const;
    void filter_naked_singles();
    void find_naked_singles();

    // hidden singles
    struct HiddenSingle {
        Coord coord;
        Value value;
        std::string tag;
    };
    friend std::ostream& operator<<(std::ostream& outs, const HiddenSingle &);
    std::vector<HiddenSingle> mHiddenSingles;

    template<class Set>
    bool test_hidden_single(const Cell &, const Value &, const Set &, std::string &) const;
    void filter_hidden_singles();
    template<class Set>
    void find_hidden_singles(const Set &);
    void find_hidden_singles();

    // naked pairs
    struct NakedPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;
    };
    friend std::ostream& operator<<(std::ostream& outs, const NakedPair &);
    std::vector<NakedPair> mNakedPairs;

    bool test_naked_pair(const Cell &, const Cell &) const;
    void filter_naked_pairs();
    template<class Set>
    void find_naked_pair(const Cell &, const Set &);
    void find_naked_pairs();

    // locked candidates
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

    void filter_locked_candidates();
    template<class Set1, class Set2>
    void find_locked_candidate(const Cell &, const Value &, Set1 &set_to_consider, Set2 &set_to_ignore);
    template<class Set>
    void find_locked_candidates(Set const &);
    void find_locked_candidates();

    // hidden pairs
    struct HiddenPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;
    };
    friend std::ostream& operator<<(std::ostream& outs, const HiddenPair &);
    std::vector<HiddenPair> mHiddenPairs;

    bool test_hidden_pair(const Cell &, const Cell &, const Value &, const Value &) const;
    void filter_hidden_pairs();
    template<class Set>
    void find_hidden_pair(const Cell &, const Value &v1, const Value &v2, const Set &);
    template<class Set>
    void find_hidden_pairs(Set const &);
    void find_hidden_pairs();

    // act
    bool act_on_naked_single();
    bool act_on_hidden_single();
    template<class Set>
    bool act_on_naked_pair(const NakedPair &, Set &);
    bool act_on_naked_pair();
    template<class Set>
    bool act_on_locked_candidate(const LockedCandidates &, Set &);
    bool act_on_locked_candidate();
    bool act_on_hidden_pair(Cell &, const HiddenPair &);
    bool act_on_hidden_pair();

private:
    using DirtySet = std::unordered_set<Coord>;
    DirtySet mValueDirtySet;    // this set of note cells is dirty because their value was set
    DirtySet mNotesDirtySet;    // this set of cells is dirty, because one of their notes was cleared

    Board *mBoard;
};
