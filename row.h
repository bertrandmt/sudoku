// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "cell.h"

#include <vector>
#include <iterator>
#include <cstddef>

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


