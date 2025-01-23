// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "coord.h"

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

class Cell;

class Notes {
public:
    Notes()
        : mNotes(kNine, true) { }

    void clear() { mNotes.clear(); }

    bool check(const Value &v) const { return mNotes.at(v - 1); }
    bool set(const Value &v, bool set);

    size_t count() const;
    std::vector<Value> values() const;

private:
    std::vector<bool> mNotes;
};

class Cell {
public:
    Cell(size_t line, size_t column)
        : mCoord(line, column)
        , mValue(kUnset) { }

    bool isNote() const { return mValue == kUnset; }
    bool isValue() const { return !isNote(); }

    const Coord &coord() const { return mCoord; }
    Value value() const { return mValue; }
    Notes &notes() { assert(isNote()); return mNotes; }
    const Notes &notes() const { assert(isNote()); return mNotes; }

    Cell &operator=(const Value &v);

    friend std::ostream& operator<< (std::ostream& outs, const Cell &);
private:
    const Coord mCoord;

    Value mValue;
    Notes mNotes;

    // horrible hack for operator<< and multiline representation
    mutable size_t mPass = 0;
};

std::ostream& operator<< (std::ostream& outs, const Cell &);
