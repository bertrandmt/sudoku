// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "cell.h"

#include <vector>
#include <iterator>
#include <cstddef>

class Nonet {
public:
    static const size_t width = 3;
    static const size_t height = 3;

    Nonet(Board &board, const Coord &coord)
        : mBoard(board)
        , mCoord((coord.row()    / height) * height,
                 (coord.column() / width)  * width) { }

    class Iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using element_type = Cell;
        using pointer = element_type *;
        using reference = element_type&;

        Iterator()
            : mStart(0, 0)
            , mSentinel(0, 0)
            , mIndex(0, 0) { throw std::runtime_error("Not implemented"); }
        Iterator(Board &board, const Coord &origin)
            : pCells(&board.mCells)
            , mWidth(board.width)
            , mStart(origin)
            , mSentinel(origin.row() + height, origin.column())
            , mIndex(origin) { }
        Iterator(const Iterator &other, const Coord &index)
            : pCells(other.pCells)
            , mWidth(other.mWidth)
            , mStart(other.mStart)
            , mSentinel(other.mSentinel)
            , mIndex(index) { }

        reference operator*() const { return pCells->at(mIndex.row() * mWidth + mIndex.column()); }

        Iterator& operator++() {
            size_t row = mIndex.row();
            size_t col = mIndex.column() + 1;
            if (col % width == 0) {
                row += 1;
                col -= width;
            }
            mIndex = Coord(row, col);
            return *this;
        }
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
        size_t mWidth;
        const Coord mStart, mSentinel;
        Coord mIndex;
    };

    auto begin() { return Iterator(mBoard, mCoord).begin(); }
    auto end() { return Iterator(mBoard, mCoord).end(); }

    auto begin() const { return Iterator(mBoard, mCoord).begin(); }
    auto end() const { return Iterator(mBoard, mCoord).end(); }

    friend std::ostream& operator<< (std::ostream& outs, const Nonet &);

private:
    Board &mBoard;
    const Coord mCoord;
};
