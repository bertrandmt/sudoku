// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <iostream>

class Coord {
public:
    Coord(size_t row, size_t column)
        : mRow(row)
        , mColumn(column) {}

    size_t row() const { return mRow; }
    size_t column() const { return mColumn; }

    bool operator==(const Coord &other) const {
        return mRow == other.mRow
            && mColumn == other.mColumn;
    }
private:
    size_t mRow;
    size_t mColumn;
};

std::ostream& operator<< (std::ostream& outs, const Coord &);
