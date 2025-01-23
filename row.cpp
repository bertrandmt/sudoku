// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "row.h"

#include <iostream>

std::ostream& operator<<(std::ostream& outs, const Row &r) {
    outs << "+-----+-----+-----++-----+-----+-----++-----+-----+-----+"
         << std::endl;
    for (auto l = 0; l < 3; l++) {
        for (auto const &c : r) {
            outs << (c.coord().column() % 3 == 0 ? "[" : "|")
                 << c
                 << (c.coord().column() % 3 == 2 ? "]" : "");
        }
        outs << std::endl;
    }
    outs << "+-----+-----+-----++-----+-----+-----++-----+-----+-----+"
         << std::endl;
    return outs;
}


