// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "cell.h"

#include <vector>
#include <iterator>
#include <cstddef>
#include <cassert>

class Row {
public:
    Row(Board &board, size_t index)
        : mBoard(board)
        , mIndex(index)
        , mCellStart(index * board.width)
        , mCellSentinel(index * board.width + board.width) { }

    std::string tag() const { return "r"; }

    class Iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using element_type = Cell;
        using pointer = element_type *;
        using reference = element_type&;

        Iterator() { throw std::runtime_error("Not implemented"); }
        Iterator(const Row &row, Board &board, size_t start)
            : mRow(&row)
            , pCells(&board.mCells)
            , mIndex(start) {
            assert(mRow->mCellStart <= mIndex);
            assert(mIndex <= mRow->mCellSentinel);
        }

        reference operator*() const { return pCells->at(mIndex); }

        Iterator& operator++() { mIndex++; assert(mIndex <= mRow->mCellSentinel); return *this; }
        Iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        bool operator==(const Iterator &other) const {
            return pCells == other.pCells
                && mIndex == other.mIndex;
        }

    private:
        const Row *mRow;
        std::vector<Cell> *pCells;
        size_t mIndex;
    };

    auto begin() { return Iterator(*this, mBoard, mCellStart); }
    auto end() { return Iterator(*this, mBoard, mCellSentinel); }

    auto begin_at(const Cell &cell) {
        assert(cell.coord().row() == mIndex);
        return Iterator(*this, mBoard, mCellStart + cell.coord().column());
    }

    auto begin() const { return Iterator(*this, mBoard, mCellStart); }
    auto end() const { return Iterator(*this, mBoard, mCellSentinel); }

    auto begin_at(const Cell &cell) const {
        assert(cell.coord().row() == mIndex);
        return Iterator(*this, mBoard, mCellStart + cell.coord().column());
    }

    bool operator==(const Row &other) const {
        return &mBoard == &other.mBoard
             && mIndex == other.mIndex;
    }

    bool operator<(const Row &other) const {
        assert(&mBoard == &other.mBoard);
        return mIndex < other.mIndex;
    }

    bool contains(const Cell &other) const {
        return std::find(begin(), end(), other) != end();
    }

    size_t index() const { return mIndex; }

    friend std::ostream& operator<< (std::ostream& outs, const Row &);

private:
    static_assert(std::forward_iterator<Iterator>);
    Board &mBoard;
    const size_t mIndex;
    const size_t mCellStart, mCellSentinel;
};
