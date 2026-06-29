// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "cell.h"

#include <cassert>

void Cell::set(const Value &v) {
    assert(isNote());

    mNotes.clear();
    mValue = v;
}

bool Notes::set(const Value &v, bool set) {
    bool note = check(v);
    if (set) mNotes |=  bit(v);
    else     mNotes &= ~bit(v);
    return note;
}

ValueList Notes::values() const {
    // value_range() is ascending, so candidates come out ascending; see the
    // declaration for why that order is an enumeration contract, not a
    // set-comparison invariant.
    ValueList v;
    for (Value value : value_range()) {
        if (check(value)) v.push_back(value);
    }
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
        if (row == 1) outs << "  " << *mValue << "  ";
        else          outs << "     ";
    }
}

size_t std::hash<Cell>::operator()(const Cell &cell) const noexcept {
    return std::hash<Coord>{}(cell.coord());
}
