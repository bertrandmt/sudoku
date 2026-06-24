// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "cell.h"

#include <cassert>

void Cell::set(const Value &v) {
    assert(isNote());
    assert(v != kUnset);

    mNotes.clear();
    mValue = v;
}

bool Notes::set(const Value &v, bool set) {

    bool note = mNotes.at(v - 1);
    mNotes[v - 1] = set;
    return note;
}

size_t Notes::count() const {
    size_t count = 0;
    for (auto const &v : mNotes) {
        if (v) count++;
    }
    return count;
}

std::vector<Value> Notes::values() const {
    std::vector<Value> v;
    if (check(kOne))   v.push_back(kOne);
    if (check(kTwo))   v.push_back(kTwo);
    if (check(kThree)) v.push_back(kThree);
    if (check(kFour))  v.push_back(kFour);
    if (check(kFive))  v.push_back(kFive);
    if (check(kSix))   v.push_back(kSix);
    if (check(kSeven)) v.push_back(kSeven);
    if (check(kEight)) v.push_back(kEight);
    if (check(kNine))  v.push_back(kNine);
    return v;
}

void Cell::print_row(std::ostream &outs, size_t row) const {
    assert(row < 3);
    if (isNote()) {
        // Row 0 -> candidates 1,2,3; row 1 -> 4,5,6; row 2 -> 7,8,9. The first
        // two positions are "* " or "  "; the last is "*" or " " (no trailing
        // space), for a 5-character field.
        const Value v1 = static_cast<Value>(row * 3 + 1);
        const Value v2 = static_cast<Value>(row * 3 + 2);
        const Value v3 = static_cast<Value>(row * 3 + 3);
        outs << (mNotes.check(v1) ? "* " : "  ")
             << (mNotes.check(v2) ? "* " : "  ")
             << (mNotes.check(v3) ? "*"  : " ");
    }
    else {
        // A solved cell shows its value centered on the middle line only.
        if (row == 1) outs << "  " << mValue << "  ";
        else          outs << "     ";
    }
}

size_t std::hash<Cell>::operator()(const Cell &cell) const noexcept {
    return std::hash<Coord>{}(cell.coord());
}
