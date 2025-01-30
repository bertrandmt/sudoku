// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#include "coord.h"

std::ostream& operator<< (std::ostream& outs, const Coord &c) {
    return outs << "[" << c.row() + 1 << ", " << c.column() + 1 << "]";
}

size_t std::hash<Coord>::operator()(const Coord &coord) const noexcept {
    return coord.row() * 9 + coord.column();
}
