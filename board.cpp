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
    bool found_naked_single = false;

    for (auto &c : *this) {
        if (c.isValue()) continue; // only considering note cells
        if (c.notes().count() != 1) continue; // this is the naked single rule: notes have only one entry

        std::vector<Value> vs = c.notes().values();
        assert(vs.size() == 1);
        Value v = vs.at(0);

        std::cout << "[NS] cell" << c.coord() << " = " << v << std::endl;
        c = v;
        autonote(c);
        found_naked_single = true;
        break;
    }

    return found_naked_single;
}

bool Board::hidden_single() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    bool found_hidden_single = false;

    for (auto &c : *this) {
        if (c.isValue()) continue; // only considering note cells

        const Row &row = rowForCell(c);
        const Column &column = columnForCell(c);
        const Nonet &nonet = nonetForCell(c);

        for (auto const &v : c.notes().values()) { // for each candidate value in this note cell
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
                found_hidden_single = true;
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
                found_hidden_single = true;
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
                found_hidden_single = true;
                break;
            }
        }
        if (found_hidden_single) break;
    }

    return found_hidden_single;
}

bool Board::locked_candidates() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
    // When a candidate is possible in a certain block and row/column, and it is not possible anywhere else in the same block,
    // then it is also not possible anywhere else in the same row/column

    bool acted_on_locked_candidates = false;

    for (auto const &c : *this) {
        if (c.isValue()) continue; // only considering note cells

        const Row &row = rowForCell(c);
        const Column &column = columnForCell(c);
        const Nonet &nonet = nonetForCell(c);

        for (auto const &v : c.notes().values()) { // for each candidate in this note cell
            // is this candidate value possible elsewhere in the same nonet
            bool condition_met = true;
            bool same_row = false;
            bool same_column = false;
            for (auto const &c1 : nonet) {
                if (same_row && same_column) {
                    // so far, we have managed to find this candidate on the same row *and* the same column,
                    // thereby breaking the condition
                    condition_met = false;
                    break;
                }

                if (c1.coord() == c.coord()) continue;                             // do not consider the current cell
                assert(c1.isNote() || c1.value() != v);
                if (c1.isValue()) continue;                                        // only considering note cells
                if (!c1.notes().check(v)) continue;                                // this note cell is _not_ a candidate for the same value
                if (rowForCell(c1) == row) { same_row = true; continue; }          // ok, but are they on the same row
                if (columnForCell(c1) == column) { same_column = true; continue; } // ok, but are they on the same column

                // we have a different note cell in the nonet, which is a candidate for this value and is not on the same row
                condition_met = false;
                break;
            }
            if (same_row && same_column) condition_met = false; // we exited the for loop just as we found a cell on the same row/column

            if (condition_met) {
                assert(same_row || same_column); // otherwise this candidte is by itself in the nonet and that should have been caught at
                                                 // naked_single or hidden_single stage
                if (same_row) {
                    for (auto &c1 : row) {
                        if (c1.coord() == c.coord()) continue;   // do not consider the current cell
                        if (c1.isValue()) continue;              // only considering note cells
                        if (!c1.notes().check(v)) continue;      // this note cell is _not_ a candidate for the same value
                        if (nonetForCell(c1) == nonet) continue; // this is another candidate in the same nonet

                        std::cout << "[LC] note" << c1.coord() << " x" << v << " [r]" << std::endl;
                        c1.notes().set(v, false);
                        acted_on_locked_candidates = true;
                    }
                }
                else {
                    assert(same_column);
                    for (auto &c1 : column) {
                        if (c1.coord() == c.coord()) continue;   // do not consider the current cell
                        if (c1.isValue()) continue;              // only considering note cells
                        if (!c1.notes().check(v)) continue;      // this note cell is _not_ a candidate for the same value
                        if (nonetForCell(c1) == nonet) continue; // this is another candidate in the same nonet

                        std::cout << "[LC] note" << c1.coord() << " x" << v << " [c]" << std::endl;
                        c1.notes().set(v, false);
                        acted_on_locked_candidates = true;
                    }
                }
            }
            if (acted_on_locked_candidates) break;
        }
        if (acted_on_locked_candidates) break;
    }

    return acted_on_locked_candidates;
}

bool Board::naked_pair() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row,
    // or column, and no other candidates are possible in those cells, then those n candidates
    // are not possible elsewhere in that same block, row, or column.
    // Applied for n = 2

    bool acted_on_naked_pair = false;

    for (auto const &c : *this) {
        if (c.isValue()) continue; // only considering note cells
        auto c_values = c.notes().values();
        assert(c_values.size() >= 2); // otherwise would have been caught at single stage.
        if (c_values.size() != 2) continue; // only considering candidates with two possible values in notes

        const Row &row = rowForCell(c);
        const Column &column = columnForCell(c);
        const Nonet &nonet = nonetForCell(c);

        auto v1 = c_values[0];
        auto v2 = c_values[1];

        // check row
        for (auto const &c1 : row) {
            if (c1.isValue()) continue;   // only considering note cells
            if (c1.coord() == c.coord()) continue; // not considering this cell
            auto c1_values = c1.notes().values();
            if (c1_values.size() != 2) continue;    // only considering other cells with two possible values in notes
            if (std::find(c1_values.begin(), c1_values.end(), v1) == c1_values.end()
             || std::find(c1_values.begin(), c1_values.end(), v2) == c1_values.end()) continue; // it's a pair, but not the same pair

            // we found a candidate pair on this row -> remove either note entry from the rest of the row
            for (auto &c2 : row) {
                if (c2.isValue()) continue;
                if (c2.coord() == c.coord()
                 || c2.coord() == c1.coord()) continue; // not looking at either of the cell pairs

                if (c2.notes().check(v1)) {
                    c2.notes().set(v1, false);
                    std::cout << "[NP] note" << c2.coord() << " x" << v1 << " [r]" << std::endl;
                    acted_on_naked_pair = true;
                }
                if (c2.notes().check(v2)) {
                    c2.notes().set(v2, false);
                    std::cout << "[NP] note" << c2.coord() << " x" << v2 << " [r]" << std::endl;
                    acted_on_naked_pair = true;
                }

            }

            if (acted_on_naked_pair) break;
        }
        if (acted_on_naked_pair) break;

        // check column
        for (auto const &c1 : column) {
            if (c1.isValue()) continue;   // only considering note cells
            if (c1.coord() == c.coord()) continue; // not considering this cell
            auto c1_values = c1.notes().values();
            if (c1_values.size() != 2) continue;    // only considering other cells with two possible values in notes
            if (std::find(c1_values.begin(), c1_values.end(), v1) == c1_values.end()
             || std::find(c1_values.begin(), c1_values.end(), v2) == c1_values.end()) continue; // it's a pair, but not the same pair

            // we found a candidate pair on this column -> remove either note entry from the rest of the column
            for (auto &c2 : column) {
                if (c2.isValue()) continue;
                if (c2.coord() == c.coord()
                 || c2.coord() == c1.coord()) continue; // not looking at either of the cell pairs

                if (c2.notes().check(v1)) {
                    c2.notes().set(v1, false);
                    std::cout << "[NP] note" << c2.coord() << " x" << v1 << " [r]" << std::endl;
                    acted_on_naked_pair = true;
                }
                if (c2.notes().check(v2)) {
                    c2.notes().set(v2, false);
                    std::cout << "[NP] note" << c2.coord() << " x" << v2 << " [r]" << std::endl;
                    acted_on_naked_pair = true;
                }

            }

            if (acted_on_naked_pair) break;
        }
        if (acted_on_naked_pair) break;

        // check nonet
        for (auto const &c1 : nonet) {
            if (c1.isValue()) continue;   // only considering note cells
            if (c1.coord() == c.coord()) continue; // not considering this cell
            auto c1_values = c1.notes().values();
            if (c1_values.size() != 2) continue;    // only considering other cells with two possible values in notes
            if (std::find(c1_values.begin(), c1_values.end(), v1) == c1_values.end()
             || std::find(c1_values.begin(), c1_values.end(), v2) == c1_values.end()) continue; // it's a pair, but not the same pair

            // we found a candidate pair on this nonet -> remove either note entry from the rest of the nonet
            for (auto &c2 : nonet) {
                if (c2.isValue()) continue;
                if (c2.coord() == c.coord()
                 || c2.coord() == c1.coord()) continue; // not looking at either of the cell pairs

                if (c2.notes().check(v1)) {
                    c2.notes().set(v1, false);
                    std::cout << "[NP] note" << c2.coord() << " x" << v1 << " [r]" << std::endl;
                    acted_on_naked_pair = true;
                }
                if (c2.notes().check(v2)) {
                    c2.notes().set(v2, false);
                    std::cout << "[NP] note" << c2.coord() << " x" << v2 << " [r]" << std::endl;
                    acted_on_naked_pair = true;
                }

            }

            if (acted_on_naked_pair) break;
        }
        if (acted_on_naked_pair) break;
    }

    return acted_on_naked_pair;
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


