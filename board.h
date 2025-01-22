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


class Row {
public:
    Row(Board &b, size_t index)
        : mBoard(b)
        , mIndex(index) { }

    class Iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using element_type = Cell;
        using pointer = element_type *;
        using reference = element_type&;

        Iterator() { throw std::runtime_error("Not implemented"); }
        Iterator(Board &board, size_t rowIndex)
            : pCells(&board.mCells)
            , mIndex(rowIndex * board.width)
            , mStart(mIndex)
            , mSentinel(mIndex + board.width) { }
        Iterator(const Iterator &i, size_t index)
            : pCells(i.pCells)
            , mIndex(index)
            , mStart(i.mStart)
            , mSentinel(i.mSentinel) { }

        reference operator*() const { return pCells->at(mIndex); }

        Iterator& operator++() { mIndex++; return *this; }
        Iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        bool operator==(const Iterator &other) const {
            return pCells == other.pCells
                && mIndex == other.mIndex;
        }

        bool operator!=(const Iterator &other) const {
            return pCells != other.pCells
                || mIndex != other.mIndex;
        }

        auto begin() { return Iterator(*this, mStart); }
        auto end() { return Iterator(*this, mSentinel); }

    private:
        std::vector<Cell> *pCells;
        size_t mIndex, mStart, mSentinel;
    };

    auto begin() { return Iterator(mBoard, mIndex).begin(); }
    auto end() { return Iterator(mBoard, mIndex).end(); }

    auto begin() const { return Iterator(mBoard, mIndex).begin(); }
    auto end() const { return Iterator(mBoard, mIndex).end(); }

    friend std::ostream& operator<< (std::ostream& outs, const Row &);

private:
    static_assert(std::forward_iterator<Iterator>);
    Board &mBoard;
    const size_t mIndex;
};

class Column {
public:
    Column(Board &b, size_t index)
        : mBoard(b)
        , mIndex(index) { }

    class Iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using element_type = Cell;
        using pointer = element_type *;
        using reference = element_type&;

        Iterator() { throw std::runtime_error("Not implemented"); }
        Iterator(Board &board, size_t columnIndex)
            : pCells(&board.mCells)
            , mIndex(columnIndex)
            , mStart(mIndex)
            , mSentinel(mIndex + board.height * board.width)
            , mStep(board.width) { }
        Iterator(const Iterator &other, size_t index)
            : pCells(other.pCells)
            , mIndex(index)
            , mStart(other.mStart)
            , mSentinel(other.mSentinel)
            , mStep(other.mStep) { }

        reference operator*() const { return pCells->at(mIndex); }

        Iterator& operator++() { mIndex += mStep; return *this; }
        Iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        bool operator==(const Iterator &other) const {
            return pCells == other.pCells
                && mIndex == other.mIndex;
        }

        bool operator!=(const Iterator &other) const {
            return pCells != other.pCells
                || mIndex != other.mIndex;
        }

        auto begin() { return Iterator(*this, mStart); }
        auto end() { return Iterator(*this, mSentinel); }

    private:
        std::vector<Cell> *pCells;
        size_t mIndex, mStart, mSentinel;
        size_t mStep;
    };

    auto begin() { return Iterator(mBoard, mIndex).begin(); }
    auto end() { return Iterator(mBoard, mIndex).end(); }

    auto begin() const { return Iterator(mBoard, mIndex).begin(); }
    auto end() const { return Iterator(mBoard, mIndex).end(); }

    friend std::ostream& operator<< (std::ostream& outs, const Column&);

private:
    Board &mBoard;
    const size_t mIndex;
};

class Nonet {
};
