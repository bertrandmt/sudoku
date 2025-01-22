// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "cell.h"

#include <cassert>

Cell &Cell::operator=(const Cell::Value &v) {
    assert(mValue == Cell::kUnset);

    mNotes.clear();
    mValue = v;

    return *this;
}

bool Cell::note(const Cell::Value &v, bool set) {
    assert(mValue == Cell::kUnset);

    bool note = mNotes.at(v - 1);
    mNotes[v - 1] = set;
    return note;
}

std::ostream& operator<< (std::ostream& outs, const Cell &c) {
    assert(c.mPass < 3);
    switch (c.mPass) {
    case 0:
        if (c.mValue == Cell::kUnset) {
            outs << (c.mNotes.at(Cell::kOne - 1)   ? "* " : "  ")
                 << (c.mNotes.at(Cell::kTwo - 1)   ? "* " : "  ")
                 << (c.mNotes.at(Cell::kThree - 1) ? "*"  : " ");
        }
        else {
            outs << "     ";
        }
        break;
    case 1:
        if (c.mValue == Cell::kUnset) {
            outs << (c.mNotes.at(Cell::kFour - 1) ? "* " : "  ")
                 << (c.mNotes.at(Cell::kFive - 1) ? "* " : "  ")
                 << (c.mNotes.at(Cell::kSix - 1)  ? "*"  : " ");
        }
        else {
            outs << "  " << c.mValue << "  ";
        }
        break;
    case 2:
        if (c.mValue == Cell::kUnset) {
            outs << (c.mNotes.at(Cell::kSeven- 1) ? "* " : "  ")
                 << (c.mNotes.at(Cell::kEight- 1) ? "* " : "  ")
                 << (c.mNotes.at(Cell::kNine- 1)  ? "*"  : " ");
        }
        else {
            outs << "     ";
        }
        break;
    default:
        assert(!"unreachable");
        return outs;
    }
    c.mPass = (c.mPass + 1) % 3;
    return outs;
}
