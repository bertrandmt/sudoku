// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"

#include <cassert>
#include <iostream>

namespace { // anonymous
    const size_t kSUDOKU_BOARD_SIZE = 81;
    //const size_t kSUDOKU_BOARD_WIDTH = 9;
    //const size_t kSUDOKU_BOARD_HEIGHT = 9;
}

Board::Board() {
    for (size_t line = 0; line < height; line++) {
        for (size_t col = 0; col < width; col++) {
            Cell c(line, col);
            mBoard.push_back(c);
        }
    }
    assert(mBoard.size() == kSUDOKU_BOARD_SIZE);
}

Cell &Board::at(size_t line, size_t col) {
    assert(line < height);
    assert(col < width);

    Cell &c = mBoard.at(line * width + col);

    return c;
}

const Cell &Board::at(size_t line, size_t col) const {
    assert(line < height);
    assert(col < width);

    const Cell &c = mBoard.at(line * width + col);

    return c;
}

void Board::autonote(size_t line, size_t col) {
    Cell &c(at(line, col));
    if (c.value() != Cell::kUnset) {
        std::cout << "Cell at [" << line + 1 << ", " << col + 1 << "] already set to " << c.value()
                  << std::endl;
        return;
    }
}

void Board::autonote() {
    for (auto c : *this) {
        if (c.value() != Cell::kUnset) {
            std::cout << "Cell at " << c.coord() << " already set to " << c.value()
                      << std::endl;
        }
    }
}

std::ostream& operator<< (std::ostream& outs, const Board &b) {
    for (size_t i = 0; i < b.height; i++) {
        outs << (i % 3 == 0 ? "+=====+=====+=====++=====+=====+=====++=====+=====+=====+\n"
                            : "+-----+-----+-----++-----+-----+-----++-----+-----+-----+\n");
        for (auto l = 0; l < 3; l++) {
            for (size_t j = 0; j < b.width; j++) {
                const Cell &c = b.at(i, j);
                outs << (j % 3 == 0 ? "[" : "|") << c << (j % 3 == 2 ? "]" : "");
            }
            outs << "\n";
        }
    }
    outs << "+=====+=====+=====++=====+=====+=====++=====+=====+=====+\n";
    return outs;
}
