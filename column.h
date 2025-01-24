// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "cell.h"

#include <vector>
#include <iterator>
#include <cstddef>

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

    bool operator==(const Column &other) const {
        return &mBoard == &other.mBoard
             && mIndex == other.mIndex;
    }

    friend std::ostream& operator<< (std::ostream& outs, const Column&);

private:
    static_assert(std::forward_iterator<Iterator>);
    Board &mBoard;
    const size_t mIndex;
};


