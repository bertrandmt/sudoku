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

private:
    const size_t mRow;
    const size_t mColumn;
};

std::ostream& operator<< (std::ostream& outs, const Coord &);
