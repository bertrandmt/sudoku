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
    for (size_t row = 0; row < height; row++) {
        for (size_t col = 0; col < width; col++) {
            Cell c(row, col);
            mCells.push_back(c);
        }

        Row r(*this, row);
        mRows.push_back(r);
    }
    assert(mCells.size() == kSUDOKU_BOARD_SIZE);
    assert(mRows.size() == height);

    for (size_t col = 0; col < width; col++) {
        Column c(*this, col);
        mColumns.push_back(c);
    }
    assert(mColumns.size() == width);

    for (size_t row = 0; row < height; row += Nonet::height) {
        for (size_t col = 0; col < width; col += Nonet::width) {
            Coord c(row, col);
            Nonet n(*this, c);
            mNonets.push_back(n);
        }
    }
    assert(mNonets.size() == (height / Nonet::height) * (width / Nonet::width));
}

Cell &Board::at(size_t row, size_t col) {
    assert(row < height);
    assert(col < width);

    Cell &c = mCells.at(row * width + col);

    return c;
}

const Cell &Board::at(size_t row, size_t col) const {
    assert(row < height);
    assert(col < width);

    const Cell &c = mCells.at(row * width + col);

    return c;
}

void Board::autonote() {
    for (auto &c : *this) {
        if (c.value() != Cell::kUnset) continue;

        // process row
        Row row(*this, c.coord().row());
        for (auto const &c1 : row) {
            if (c1.value() == Cell::kUnset) continue;
            if (!c.note(c1.value())) continue;

            std::cout << "[AN] note" << c.coord() << " XXX " << c1.value()
                      << " in row at " << c1.coord() << std::endl;
            c.note(c1.value(), false);
        }
        // process column
        Column col(*this, c.coord().column());
        for (auto const &c1 : col) {
            if (c1.value() == Cell::kUnset) continue;
            if (!c.note(c1.value())) continue;

            std::cout << "[AN] note" << c.coord() << " XXX " << c1.value()
                      << " in column at " << c1.coord() << std::endl;
            c.note(c1.value(), false);
        }
        // process nonet
        Nonet nonet(*this, c.coord());
        for (auto const &c1 : nonet) {
            if (c1.value() == Cell::kUnset) continue;
            if (!c.note(c1.value())) continue;

            std::cout << "[AN] note" << c.coord() << " XXX " << c1.value()
                      << " in nonet at " << c1.coord() << std::endl;
            c.note(c1.value(), false);
        }
    }
}

std::ostream& operator<<(std::ostream& outs, const Board &b) {
    for (size_t i = 0; i < b.height; i++) {
        outs << (i % 3 == 0 ? "+=====+=====+=====++=====+=====+=====++=====+=====+=====+"
                            : "+-----+-----+-----++-----+-----+-----++-----+-----+-----+")
             << std::endl;
        for (auto l = 0; l < 3; l++) {
            for (size_t j = 0; j < b.width; j++) {
                const Cell &c = b.at(i, j);
                outs << (j % 3 == 0 ? "[" : "|") << c << (j % 3 == 2 ? "]" : "");
            }
            outs << std::endl;
        }
    }
    outs << "+=====+=====+=====++=====+=====+=====++=====+=====+=====+"
         << std::endl;
    return outs;
}


