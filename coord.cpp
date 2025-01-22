// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include "coord.h"

std::ostream& operator<< (std::ostream& outs, const Coord &c) {
    return outs << "[" << c.line() << ", " << c.column() << "]";
}

