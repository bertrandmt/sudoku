// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "column.h"

#include <iostream>

std::ostream& operator<<(std::ostream& outs, const Column &col) {
    for (auto const &c : col) {
        outs << "+-----+" << std::endl;
        for (auto l = 0; l < 3; l++) {
            outs << "|" << c << "|" << std::endl;
        }
    }
    outs << "+-----+" << std::endl;
    return outs;
}
