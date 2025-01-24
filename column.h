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
    Column(Board &board, size_t index)
        : mBoard(board)
        , mIndex(index)
        , mCellStart(index) // line 0, column 'index'
        , mCellSentinel(index + board.height * board.width) { }

    std::string tag() const { return "c"; }

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
            , mStep(board.width) { }

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

    private:
        std::vector<Cell> *pCells;
        size_t mIndex;
        size_t mStep;
    };

    auto begin() { return Iterator(mBoard, mCellStart); }
    auto end() { return Iterator(mBoard, mCellSentinel); }

    auto begin() const { return Iterator(mBoard, mCellStart); }
    auto end() const { return Iterator(mBoard, mCellSentinel); }

    bool operator==(const Column &other) const {
        return &mBoard == &other.mBoard
             && mIndex == other.mIndex;
    }

    friend std::ostream& operator<< (std::ostream& outs, const Column&);

private:
    static_assert(std::forward_iterator<Iterator>);
    Board &mBoard;
    const size_t mIndex;
    const size_t mCellStart, mCellSentinel;
};
