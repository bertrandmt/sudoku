// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "cell.h"

#include <cassert>

Cell &Cell::operator=(const Value &v) {
    assert(mValue == kUnset);

    mNotes.clear();
    mValue = v;

    return *this;
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

std::ostream& operator<< (std::ostream& outs, const Cell &c) {
    assert(c.mPass < 3);
    switch (c.mPass) {
    case 0:
        if (c.mValue == kUnset) {
            outs << (c.mNotes.check(kOne)   ? "* " : "  ")
                 << (c.mNotes.check(kTwo)   ? "* " : "  ")
                 << (c.mNotes.check(kThree) ? "*"  : " ");
        }
        else {
            outs << "     ";
        }
        break;
    case 1:
        if (c.mValue == kUnset) {
            outs << (c.mNotes.check(kFour) ? "* " : "  ")
                 << (c.mNotes.check(kFive) ? "* " : "  ")
                 << (c.mNotes.check(kSix)  ? "*"  : " ");
        }
        else {
            outs << "  " << c.mValue << "  ";
        }
        break;
    case 2:
        if (c.mValue == kUnset) {
            outs << (c.mNotes.check(kSeven) ? "* " : "  ")
                 << (c.mNotes.check(kEight) ? "* " : "  ")
                 << (c.mNotes.check(kNine)  ? "*"  : " ");
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
