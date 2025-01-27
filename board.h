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

    template<class Set1, class Set2>
    bool act_on_locked_candidates(const Cell &, const Value &, const Set1 &, const Set2 &);
    template<class Set1, class Set2>
    bool locked_candidates(const Cell &, const Value &, Set1 &, Set2 &);
    bool locked_candidates();

    template<class Set>
    bool naked_pair(const Cell &, Set &);
    bool naked_pair();

    template<class Set>
    bool hidden_pair(Cell &, const Value &, const Value &, Set &);
    bool hidden_pair();


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

    std::vector<Coord> mNakedSingles;
    struct HiddenSingle {
        Coord coord;
        Value value;
        std::string tag;
    };
    std::vector<HiddenSingle> mHiddenSingles;

    template<class Set>
    void autonote(Cell &, Set &);
    void autonote(Cell &);

    template<class Set>
    void find_naked_singles_in_set(const Set &);
    void find_naked_singles(const Cell &);
    void find_naked_singles();

    template<class Set>
    bool test_hidden_single(const Cell &, const Value &, const Set &, /*out*/std::string &) const;
    template<class Set>
    void find_hidden_singles_in_set(const Set &);
    void find_hidden_singles(const Cell &);
    void find_hidden_singles();

    void find_locked_candidates();

    void analyze(const Cell &);
    void analyze();

    void rebuild_subsets();
};

#include "row.h"
#include "column.h"
#include "nonet.h"
