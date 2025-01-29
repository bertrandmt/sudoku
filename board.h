// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"

#include <vector>
#include <iterator>
#include <cstddef>

class Row;
class Column;
class Nonet;

class Board {
public:
    Board();
    Board(const Board &other);

    static const size_t width = 9;
    static const size_t height = 9;

    void print(std::ostream &out);

    Cell &at(size_t row, size_t col);
    const Cell &at(size_t row, size_t col) const;
    Cell &at(const Coord &coord) { return at(coord.row(), coord.column()); }
    const Cell &at(const Coord &coord) const { return at(coord.row(), coord.column()); }

    const Row &row(const Cell &) const;
    const Column &column(const Cell &) const;
    const Nonet &nonet(const Cell &) const;

    void autonote();

    bool act_on_naked_single();
    bool act_on_hidden_single();
    bool act_on_locked_candidate();
    bool act_on_naked_pair();
    bool act_on_hidden_pair();

    std::vector<Cell> &cells() { return mCells; }
    const std::vector<Cell> &cells() const { return mCells; }

    std::vector<Row> &rows() { return mRows; }
    const std::vector<Row> &rows() const { return mRows; }

    std::vector<Column> &columns() { return mColumns; }
    const std::vector<Column> &columns() const { return mColumns; }

    std::vector<Nonet> &nonets() { return mNonets; }
    const std::vector<Nonet> &nonets() const { return mNonets; }

    friend std::ostream& operator<< (std::ostream& outs, const Board &);
    friend class Row;
    friend class Column;
    friend class Nonet;

private:
    std::vector<Cell> mCells;
    std::vector<Row> mRows;
    std::vector<Column> mColumns;
    std::vector<Nonet> mNonets;

    template<class Set>
    void autonote(Cell &, Set &);
    void autonote(Cell &);

    // naked singles
    std::vector<Coord> mNakedSingles;
    template<class Set>
    void find_naked_singles_in_set(const Set &);
    void find_naked_singles(const Cell &);
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
    bool test_hidden_single(const Cell &, const Value &, const Set &, /*out*/std::string &) const;
    template<class Set>
    void find_hidden_singles_in_set(const Set &);
    void find_hidden_singles(const Cell &);
    void find_hidden_singles();

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

    template<class Set>
    bool act_on_locked_candidate(const LockedCandidates &, Set &);
    template<class Set1, class Set2>
    bool test_locked_candidate(const Cell &, const Value &, Set1 &set_to_consider, Set2 &set_to_ignore);
    template<class Set>
    void find_locked_candidates_in_set(const Set &);
    void find_locked_candidates(const Cell &);
    void find_locked_candidates();

    // naked pairs
    struct NakedPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;
    };
    friend std::ostream& operator<<(std::ostream& outs, const NakedPair &);
    std::vector<NakedPair> mNakedPairs;

    template<class Set>
    bool act_on_naked_pair(const NakedPair &, Set &);
    template<class Set>
    void find_naked_pair(const Cell &, const Set &);
    template<class Set>
    void find_naked_pairs_in_set(const Set &);
    void find_naked_pairs(const Cell &);
    void find_naked_pairs();

    // hidden pairs
    struct HiddenPair {
        std::pair<Coord, Coord> coords;
        std::pair<Value, Value> values;
    };
    friend std::ostream& operator<<(std::ostream& outs, const HiddenPair &);
    std::vector<HiddenPair> mHiddenPairs;

    bool act_on_hidden_pair(Cell &, const HiddenPair &);
    template<class Set>
    void test_hidden_pair(const Cell &, const Value &, const Value &, const Set &);
    bool test_hidden_pair(const HiddenPair &);
    template<class Set>
    void test_hidden_pairs_in_set(const Set &);
    template<class Set>
    void find_hidden_pairs_in_set(const Set &);
    void find_hidden_pairs(const Cell &);
    void find_hidden_pairs();

    void analyze(const Cell &);
    void analyze();

    void rebuild_subsets();
};

#include "row.h"
#include "column.h"
#include "nonet.h"
