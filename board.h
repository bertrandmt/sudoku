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

    static const size_t width = 9;
    static const size_t height = 9;

    Cell &at(size_t row, size_t col);
    const Cell &at(size_t row, size_t col) const;

    const Row &rowForCell(const Cell &) const;
    const Column &columnForCell(const Cell &) const;
    const Nonet &nonetForCell(const Cell &) const;

    template<class Set>
    void autonote(Cell &, Set &);
    void autonote(Cell &);
    void autonote();

    bool naked_single();

    template<class Set>
    bool hidden_single(Cell &, const Set &, const Value &);
    bool hidden_single();

    template<class Set>
    bool act_on_locked_candidates(const Cell &, const Value &, const Set &);
    bool locked_candidates();
    bool naked_pair();
    bool hidden_pair();


    auto begin() { return mCells.begin(); }
    auto end() { return mCells.end(); }

    auto begin() const { return mCells.begin(); }
    auto end() const { return mCells.end(); }


    auto row_begin() { return mRows.begin(); }
    auto row_end() { return mRows.end(); }

    auto row_begin() const { return mRows.begin(); }
    auto row_end() const { return mRows.end(); }


    auto column_begin() { return mColumns.begin(); }
    auto column_end() { return mColumns.end(); }

    auto column_begin() const { return mColumns.begin(); }
    auto column_end() const { return mColumns.end(); }


    auto nonet_begin() { return mNonets.begin(); }
    auto nonet_end() { return mNonets.end(); }

    auto nonet_begin() const { return mNonets.begin(); }
    auto nonet_end() const { return mNonets.end(); }


    friend std::ostream& operator<< (std::ostream& outs, const Board &);
    friend class Row;
    friend class Column;
    friend class Nonet;

private:
    std::vector<Cell> mCells;
    std::vector<Row> mRows;
    std::vector<Column> mColumns;
    std::vector<Nonet> mNonets;
};

#include "row.h"
#include "column.h"
#include "nonet.h"
