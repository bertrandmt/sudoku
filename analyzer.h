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

    using ptr = std::shared_ptr<Analyzer>;

    void analyze(Board &board);

    void value_dirty(const Cell &);
    void notes_dirty(const Cell &);

private:
    template<class Set>
    void filter_notes_for_set(Cell &, const Set &);
    template<class Set>
    void filter_notes(const Set &);
    void filter_notes();

    // naked singles
    bool test_naked_single(const Cell &) const;
    void filter_naked_singles() const;
    void find_naked_singles() const;

    // hidden singles
    template<class Set>
    bool test_hidden_single(const Cell &, const Value &, const Set &, std::string &) const;
    void filter_hidden_singles() const;
    template<class Set>
    void find_hidden_singles(const Set &) const;
    void find_hidden_singles() const;

    // naked pairs
    bool test_naked_pair(const Cell &, const Cell &) const;
    void filter_naked_pairs() const;
    template<class Set>
    void find_naked_pair(const Cell &, const Set &) const;
    void find_naked_pairs() const;

    // locked candidates
    template<class Set1, class Set2>
    void find_locked_candidate(const Cell &, const Value &, Set1 &set_to_consider, Set2 &set_to_ignore) const;
    void filter_locked_candidates() const;
    void find_locked_candidates() const;

    // hidden pairs
    bool test_hidden_pair(const Cell &, const Cell &, const Value &, const Value &) const;
    void filter_hidden_pairs() const;
    template<class Set>
    void find_hidden_pair(const Cell &, const Value &v1, const Value &v2, const Set &) const;
    template<class Set>
    void find_hidden_pairs(Set const &) const;
    void find_hidden_pairs() const;

private:
    using DirtySet = std::unordered_set<Coord>;
    DirtySet mValueDirtySet;    // this set of note cells is dirty because their value was set
    DirtySet mNotesDirtySet;    // this set of cells is dirty, because one of their notes was cleared

    Board *mBoard;
};
