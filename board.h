// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"

#include <vector>
#include <iterator>
#include <cstddef>

class Row {
public:

private:
};

class Board {
public:
    Board();

    const size_t width = 9;
    const size_t height = 9;

    Cell &at(size_t line, size_t col);
    const Cell &at(size_t line, size_t col) const;

    void autonote(size_t line, size_t col);
    void autonote();

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Cell;
        using difference_type = std::ptrdiff_t;
        using pointer = Cell *;
        using reference = Cell&;

        Iterator(std::vector<Cell> &board, size_t idx)
            : pBoard(&board)
            , mIndex(idx) {}

        reference operator*() const { return pBoard->at(mIndex); }
        pointer operator->() const { return &pBoard->at(mIndex); }

        Iterator& operator++() {
            mIndex++;
            return *this;
        }

        bool operator==(const Iterator &other) const {
            return pBoard == other.pBoard
                && mIndex == other.mIndex;
        }

        bool operator!=(const Iterator &other) const {
            return pBoard != other.pBoard
                || mIndex != other.mIndex;
        }

    private:
        std::vector<Cell> *pBoard;
        size_t mIndex;
    };

    Iterator begin() { return Iterator(mBoard, 0); }
    Iterator end() { return Iterator(mBoard, mBoard.size()); }

    friend std::ostream& operator<< (std::ostream& outs, const Board &);

private:
    std::vector<Cell> mBoard;
};

std::ostream& operator<< (std::ostream& outs, const Board &);
