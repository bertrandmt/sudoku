// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <string>
#include <vector>

#include "coord.h"

class Cell {
public:
    enum Value: int {
        kUnset = 0,
        kOne,
        kTwo,
        kThree,
        kFour,
        kFive,
        kSix,
        kSeven,
        kEight,
        kNine,
    };

    Cell(size_t line, size_t column)
        : mCoord(line, column)
        , mValue(kUnset)
        , mNotes(kNine, true) { }

    const Coord &coord() const { return mCoord; }
    Value value() const { return mValue; }
    std::vector<bool> notes() const { return mNotes; }

    Cell &operator=(const Value &v);
    bool note(const Value &v, bool set);

    friend std::ostream& operator<< (std::ostream& outs, const Cell &);
private:
    const Coord mCoord;

    Value mValue;
    std::vector<bool> mNotes;

    mutable size_t mPass = 0;
};

std::ostream& operator<< (std::ostream& outs, const Cell &);
