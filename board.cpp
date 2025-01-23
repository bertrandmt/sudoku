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

const Row &Board::rowForCell(const Cell &c) const {
    return mRows.at(c.coord().row());
}

const Column &Board::columnForCell(const Cell &c) const {
    return mColumns.at(c.coord().column());
}

const Nonet &Board::nonetForCell(const Cell &c) const {
    size_t nonetRow = (c.coord().row() / 3) * 3;
    size_t nonetCol = (c.coord().column() / 3) * 3;

    return mNonets.at(nonetRow + nonetCol / 3);
}

void Board::autonote(Cell &c) {
    if (c.isNote()) {
        // this is a note cell; let's update its own notes from all the value cells in the same row, column and nonet

        // process row
        Row row(*this, c.coord().row());
        for (auto const &c1 : row) {
            if (c1.isNote()) continue; // other note cells do not participate in this update
            if (!c.notes().check(c1.value())) continue; // the value of c1 is already checked off c's notes

            std::cout << "  [ANn] note" << c.coord() << " X" << c1.value()
                      << " r(" << c1.coord() << ")" << std::endl;
            c.notes().set(c1.value(), false);
        }
        // process column
        Column col(*this, c.coord().column());
        for (auto const &c1 : col) {
            if (c1.isNote()) continue; // other note cells do not participate in this update
            if (!c.notes().check(c1.value())) continue;

            std::cout << "  [ANn] note" << c.coord() << " X" << c1.value()
                      << " c(" << c1.coord() << ")" << std::endl;
            c.notes().set(c1.value(), false);
        }
        // process nonet
        Nonet nonet(*this, c.coord());
        for (auto const &c1 : nonet) {
            if (c1.isNote()) continue; // other note cells do not participate in this update
            if (!c.notes().check(c1.value())) continue;

            std::cout << "  [ANn] note" << c.coord() << " X" << c1.value()
                      << " n(" << c1.coord() << ")" << std::endl;
            c.notes().set(c1.value(), false);
        }
    }
    else {
        // this is a value cell; let's update notes in note cells in the same row, column and nonet

        // process row
        Row row(*this, c.coord().row());
        for (auto &c1 : row) {
            if (c1.isValue()) continue; // other value cells do not get changed by this process
            if (!c1.notes().check(c.value())) continue; // the value of c is already checked off c1's notes

            std::cout << "  [ANv] note" << c1.coord() << " X" << c.value()
                      << " r(" << c.coord() << ")" << std::endl;
            c1.notes().set(c.value(), false);
        }
        // process column
        Column col(*this, c.coord().column());
        for (auto &c1 : col) {
            if (c1.isValue()) continue; // other value cells do not get changed by this process
            if (!c1.notes().check(c.value())) continue; // the value of c is already checked off c1's notes

            std::cout << "  [ANv] note" << c1.coord() << " X" << c.value()
                      << " c(" << c.coord() << ")" << std::endl;
            c1.notes().set(c.value(), false);
        }
        // process nonet
        Nonet nonet(*this, c.coord());
        for (auto &c1 : nonet) {
            if (c1.isValue()) continue; // other value cells do not get changed by this process
            if (!c1.notes().check(c.value())) continue; // the value of c is already checked off c1's notes

            std::cout << "  [ANv] note" << c1.coord() << " X" << c.value()
                      << " n(" << c.coord() << ")" << std::endl;
            c1.notes().set(c.value(), false);
        }
     }
}

void Board::autonote() {
    for (auto &c : *this) {
        autonote(c);
    }
}

bool Board::naked_single() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A naked single arises when there is only one possible candidate for a cell
    size_t naked_single_count(0);

    for (auto &c : *this) {
        if (c.isValue()) continue; // only considering note cells
        if (c.notes().count() != 1) continue; // this is the naked single rule: notes have only one entry

        std::vector<Value> vs = c.notes().values();
        assert(vs.size() == 1);
        Value v = vs.at(0);

        std::cout << "[NS] cell" << c.coord() << " = " << v << std::endl;
        c = v;
        autonote(c);
        naked_single_count++;
    }

    return naked_single_count != 0;
}

bool Board::hidden_single() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    size_t hidden_single_count(0);

    for (auto &c : *this) {
        if (c.isValue()) continue; // only considering note cells

        const Row &row = rowForCell(c);
        const Column &column = columnForCell(c);
        const Nonet &nonet = nonetForCell(c);

        std::vector<Value> vs = c.notes().values();
        for (auto const &v : vs) { // for each candidate value in this cell
            // check this row
            bool other_possible_cell_candidate = false;
            for (auto const &c1 : row) {
                if (c1.coord() == c.coord()) continue;  // do not consider the current cell
                assert(c1.isNote() || c1.value() != v);
                if (c1.isValue()) continue;             // only considering note cells
                if (!c1.notes().check(v)) continue;     // this note cell is _not_ a candidate

                other_possible_cell_candidate = true;
                break;
            }
            if (!other_possible_cell_candidate) {
                std::cout << "[HS] cell" << c.coord() << " = " << v << " [r]" << std::endl;
                c = v;
                autonote(c);
                hidden_single_count++;
                break;
            }

            // check this column
            other_possible_cell_candidate = false;
            for (auto const &c1 : column) {
                if (c1.coord() == c.coord()) continue;  // do not consider the current cell
                assert(c1.isNote() || c1.value() != v);
                if (c1.isValue()) continue;             // only considering note cells
                if (!c1.notes().check(v)) continue;     // this note cell is _not_ a candidate

                other_possible_cell_candidate = true;
                break;
            }
            if (!other_possible_cell_candidate) {
                std::cout << "[HS] cell" << c.coord() << " = " << v << " [c]" << std::endl;
                c = v;
                autonote(c);
                hidden_single_count++;
                break;
            }

            // check this nonet
            other_possible_cell_candidate = false;
            for (auto const &c1 : nonet) {
                if (c1.coord() == c.coord()) continue;  // do not consider the current cell
                assert(c1.isNote() || c1.value() != v);
                if (c1.isValue()) continue;             // only considering note cells
                if (!c1.notes().check(v)) continue;     // this note cell is _not_ a candidate

                other_possible_cell_candidate = true;
                break;
            }
            if (!other_possible_cell_candidate) {
                std::cout << "[HS] cell" << c.coord() << " = " << v << " [n]" << std::endl;
                c = v;
                autonote(c);
                hidden_single_count++;
                break;
            }
        }
    }

    return hidden_single_count != 0;
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
    outs << "+=====+=====+=====++=====+=====+=====++=====+=====+=====+";
    return outs;
}


