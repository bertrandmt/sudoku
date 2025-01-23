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

    const size_t width = 9;
    const size_t height = 9;

    Cell &at(size_t row, size_t col);
    const Cell &at(size_t row, size_t col) const;

    void autonote(size_t row, size_t col);
    void autonote();

    //***
    // Cell iterator over all cells
    using Iterator = std::vector<Cell>::iterator;

    auto begin() { return mCells.begin(); }
    auto end() { return mCells.end(); }

    auto begin() const { return mCells.begin(); }
    auto end() const { return mCells.end(); }

    //***
    // Row iterator
    using RowIterator = std::vector<Row>::iterator;

    auto row_begin() { return mRows.begin(); }
    auto row_end() { return mRows.end(); }

    auto row_begin() const { return mRows.begin(); }
    auto row_end() const { return mRows.end(); }

    //***
    // Column iterator
    using ColumnIterator = std::vector<Column>::iterator;

    auto column_begin() { return mColumns.begin(); }
    auto column_end() { return mColumns.end(); }

    auto column_begin() const { return mColumns.begin(); }
    auto column_end() const { return mColumns.end(); }

    friend std::ostream& operator<< (std::ostream& outs, const Board &);
    friend class Row;
    friend class Column;

private:
    static_assert(std::forward_iterator<Iterator>);
    std::vector<Cell> mCells;
    std::vector<Row> mRows;
    std::vector<Column> mColumns;
};

#include "row.h"
#include "column.h"
#include "nonet.h"
