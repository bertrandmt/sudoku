// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <iostream>

class Coord {
public:
    Coord(size_t line, size_t column)
        : mLine(line)
        , mColumn(column) {}

    size_t line() const { return mLine; }
    size_t column() const { return mColumn; }

private:
    const size_t mLine;
    const size_t mColumn;
};

std::ostream& operator<< (std::ostream& outs, const Coord &);
