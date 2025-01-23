// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "row.h"

#include <cassert>
#include <iostream>

std::ostream& operator<<(std::ostream& outs, const Nonet &n) {
    auto pc = n.begin();

    for (size_t i = 0; i < n.height; i++) {
        outs << "+-----+-----+-----+" << std::endl;
        for (auto l = 0; l < 3; l++) {
            auto lpc = pc;
            for (size_t j = 0; j < n.width; j++) {
                outs << "|" << *lpc;
                lpc++;
            }
            outs << "|" << std::endl;
        }
        for (size_t j = 0; j < n.width; j++) { pc++; }
    }
    outs << "+-----+-----+-----+" << std::endl;
    assert(pc == n.end());
    return outs;
}
