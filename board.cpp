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

const Row &Board::row(const Cell &c) const {
    return mRows.at(c.coord().row());
}

const Column &Board::column(const Cell &c) const {
    return mColumns.at(c.coord().column());
}

const Nonet &Board::nonet(const Cell &c) const {
    size_t nonetRow = (c.coord().row() / 3) * 3;
    size_t nonetCol = (c.coord().column() / 3) * 3;

    return mNonets.at(nonetRow + nonetCol / 3);
}

template<class Set>
void Board::autonote(Cell &cell, Set &set) {
    assert(std::find(set.begin(), set.end(), cell) != set.end());

    if (cell.isNote()) {
        // this is a note cell; let's update its own notes from all the value cells in the same set
        for (auto const &other_cell : set) {
            if (other_cell.isNote()) continue; // other note cells do not participate in this update
            if (!cell.notes().check(other_cell.value())) continue; // the value of other_cell is already checked off cell's notes

            std::cout << "  [ANn] note" << cell.coord() << " x" << other_cell.value()
                      << " " << set.tag() << "(" << other_cell.coord() << ")" << std::endl;
            cell.notes().set(other_cell.value(), false);
        }
    }
    else {
        assert(cell.isValue());
        // this is a value cell; let's update notes in note cells in the same set
        for (auto &other_cell : set) {
            if (other_cell.isValue()) continue; // other value cells do not get changed by this process
            if (!other_cell.notes().check(cell.value())) continue; // the value of cell is already checked off other_cell's notes

            std::cout << "  [ANv] note" << other_cell.coord() << " x" << cell.value()
                      << " " << set.tag() << "(" << cell.coord() << ")" << std::endl;
            other_cell.notes().set(cell.value(), false);
        }
    }
}

void Board::autonote(Cell &cell) {
    autonote(cell, row(cell));
    autonote(cell, column(cell));
    autonote(cell, nonet(cell));
}

void Board::autonote() {
    for (auto &cell : *this) {
        autonote(cell);
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

        std::cout << "[NS] cell" << c.coord() << " =" << v << std::endl;
        c = v;
        autonote(c);
        found_naked_single = true;
        break;
    }

    return found_naked_single;
}

template<class Set>
bool Board::hidden_single(Cell &cell, const Set &set, const Value &value) {
    bool other_possible_cell_candidate = false;

    for (auto const &other_cell : set) {
        if (other_cell == cell) continue;               // do not consider the current cell
        assert(other_cell.isNote() || other_cell.value() != value);
        if (other_cell.isValue()) continue;             // only considering note cells
        if (!other_cell.notes().check(value)) continue; // this note cell is _not_ a candidate

        other_possible_cell_candidate = true;
        break;
    }
    if (!other_possible_cell_candidate) {
        std::cout << "[HS] cell" << cell.coord() << " =" << value << " [" << set.tag() << "]" << std::endl;
        cell = value;
        autonote(cell);
        return true;
    }
    return false;
}

bool Board::hidden_single() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    bool found_hidden_single = false;

    for (auto &cell : *this) {
        if (cell.isValue()) continue; // only considering note cells

        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell

            if (hidden_single(cell, row(cell), value)) { found_hidden_single = true; break; }
            if (hidden_single(cell, column(cell), value)) { found_hidden_single = true; break; }
            if (hidden_single(cell, nonet(cell), value)) { found_hidden_single = true; break; }
        }
        if (found_hidden_single) break;
    }

    return found_hidden_single;
}

template<class Set>
bool Board::act_on_locked_candidates(const Cell &cell, const Value &value, const Set &set) {
    bool acted_on_locked_candidates = false;

    for (auto &other_cell : set) {
        if (other_cell == cell) continue;   // do not consider the current cell
        if (other_cell.isValue()) continue;              // only considering note cells
        if (!other_cell.notes().check(value)) continue;      // this note cell is _not_ a candidate for the same value
        if (nonet(other_cell) == nonet(cell)) continue; // this is another candidate in the same nonet

        std::cout << "[LC] note" << other_cell.coord() << " x" << value << " [" << set.tag() << "]" << std::endl;
        other_cell.notes().set(value, false);
        acted_on_locked_candidates = true;
    }

    return acted_on_locked_candidates;
}

bool Board::locked_candidates() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
    // When a candidate is possible in a certain block and row/column, and it is not possible anywhere else in the same block,
    // then it is also not possible anywhere else in the same row/column

    bool acted_on_locked_candidates = false;

    for (auto const &c : *this) {
        if (c.isValue()) continue; // only considering note cells

        for (auto const &v : c.notes().values()) { // for each candidate value in this note cell
            bool condition_met = true;
            bool same_row = false;
            bool same_column = false;

            // is this candidate value possible elsewhere in the same nonet
            for (auto const &c1 : nonet(c)) {
                if (same_row && same_column) { condition_met = false; break; }

                if (c1 == c) continue;                             // do not consider the current cell
                assert(c1.isNote() || c1.value() != v);
                if (c1.isValue()) continue;                                        // only considering note cells
                if (!c1.notes().check(v)) continue;                                // this note cell is _not_ a candidate for the same value
                if (row(c1) == row(c)) { same_row = true; continue; }          // ok, but are they on the same row
                if (column(c1) == column(c)) { same_column = true; continue; } // ok, but are they on the same column

                // we have a different note cell in the nonet, which is a candidate for this value and is not on the same row
                condition_met = false;
                break;
            }
            if (same_row && same_column) condition_met = false; // we exited the for loop just as we found a cell on the same row/column
            if (!same_row && !same_column) condition_met = false; // this candidate is by itself in the nonet and this is for naked_single or hidden_single to handle

            if (condition_met) {
                if (same_row) {
                    acted_on_locked_candidates = act_on_locked_candidates(c, v, row(c));
                }
                else {
                    assert(same_column);
                    acted_on_locked_candidates = act_on_locked_candidates(c, v, column(c));
                }
            }
            if (acted_on_locked_candidates) break;
        }
        if (acted_on_locked_candidates) break;
    }

    return acted_on_locked_candidates;
}

template<class Set>
bool Board::naked_pair(const Cell &cell, Set &set) {
    bool acted_on_naked_pair = false;

    assert(cell.isNote());
    auto c_values = cell.notes().values();
    assert(c_values.size() == 2);

    auto v1 = c_values[0];
    auto v2 = c_values[1];

    for (auto const &pair_cell : set) {
        if (pair_cell.isValue()) continue;   // only considering note cells
        if (pair_cell == cell) continue; // not considering this cell
        auto pair_cell_values = pair_cell.notes().values();
        if (pair_cell_values.size() != 2) continue;    // only considering other cells with two possible values in notes
        if (std::find(pair_cell_values.begin(), pair_cell_values.end(), v1) == pair_cell_values.end()
         || std::find(pair_cell_values.begin(), pair_cell_values.end(), v2) == pair_cell_values.end()) continue; // it's a pair, but not the same pair

        // we found a candidate pair in this set -> remove either note entry from the rest of the set
        for (auto &other_cell : set) {
            if (other_cell.isValue()) continue;
            if (other_cell == cell || other_cell == pair_cell) continue; // not looking at either of the cell pairs

            if (other_cell.notes().check(v1)) {
                other_cell.notes().set(v1, false);
                std::cout << "[NP] note" << other_cell.coord() << " x" << v1 << " [" << set.tag() << "]" << std::endl;
                acted_on_naked_pair = true;
            }
            if (other_cell.notes().check(v2)) {
                other_cell.notes().set(v2, false);
                std::cout << "[NP] note" << other_cell.coord() << " x" << v2 << " [" << set.tag() << "]" << std::endl;
                acted_on_naked_pair = true;
            }
        }

        if (acted_on_naked_pair) break;
    }

    return acted_on_naked_pair;
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

        if (naked_pair(c, row(c))) { acted_on_naked_pair = true; break; }
        if (naked_pair(c, column(c))) { acted_on_naked_pair = true; break; }
        if (naked_pair(c, nonet(c))) { acted_on_naked_pair = true; break; }
    }

    return acted_on_naked_pair;
}

template<class Set>
bool Board::hidden_pair(Cell &cell, const Value &v1, const Value &v2, Set &set, bool &consider_next_pv1) {
    assert(cell.isNote());
    assert(cell.notes().check(v1));
    assert(cell.notes().check(v2));

    bool acted_on_hidden_pair = false;

    // can we find another note cell with the same pair in the same set, but no other cell with either candidate in the set?
    Cell *ppair_cell = NULL; // "the" other potential candidate
    bool condition_met = true;

    for (auto &other_cell : set) {
        if (other_cell.isValue()) continue; // only considering note cells
        if (other_cell == cell) continue;      // not considering this cell
        if (!other_cell.notes().check(v1) && !other_cell.notes().check(v2)) continue; // no impact on algorithm; check next cell in row
        if (other_cell.notes().check(v1) && other_cell.notes().check(v2)) {
            if (!ppair_cell) { // no candidate yet
                ppair_cell = &other_cell; // this is "the" other candidate
            }
            else { // this is disqualifying: we have more than two candidates in the row
                condition_met = false;
                break;
            }
        }
        if (other_cell.notes().check(v2) && !other_cell.notes().check(v1)) { // this is disqualifying for pv2
            condition_met = false;
            break;
        }
        if (other_cell.notes().check(v1) && !other_cell.notes().check(v2)) { // this is disqualifying for pv1

            condition_met = false;
            consider_next_pv1 = true;
            break;
        }
    }
    if (consider_next_pv1) { assert(!condition_met); }
    if (!ppair_cell) { condition_met = false; } // we did not, in fact, find another candidate
    if (!condition_met) { return acted_on_hidden_pair; }

    if (cell.notes().values().size() == 2 && ppair_cell->notes().values().size() == 2) { // condition was met, in a way, but there is no action to take
        return acted_on_hidden_pair;
    }

    // we have a pair of cells cell,*ppair_cell with a pair of values v1,v2 and nobody else in the row has those values
    cell.notes().set_all(false);
    cell.notes().set(v1, true);
    cell.notes().set(v2, true);
    std::cout << "[HP] note" << cell.coord() << " ={" << v1 << "," << v2 << "} [" << set.tag() << "]" << std::endl;

    assert(ppair_cell);
    ppair_cell->notes().set_all(false);
    ppair_cell->notes().set(v1, true);
    ppair_cell->notes().set(v2, true);
    std::cout << "[HP] note" << ppair_cell->coord() << " ={" << v1 << "," << v2 << "} [" << set.tag() << "]" << std::endl;

    acted_on_hidden_pair = true;
    return acted_on_hidden_pair;
}

bool Board::hidden_pair() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row, or column,
    // and those n candidates are not possible elsewhere in that same block, row, or column, then no other
    // candidates are possible in those cells.
    // Applied for n = 2

    bool acted_on_hidden_pair = false;

    for (auto &c : *this) {
        if (c.isValue()) continue; // only considering note cells
        auto c_values = c.notes().values();
        assert(c_values.size() >= 2); // otherwise would have been caught at single stage.

        for (auto pv1 = c_values.begin(); pv1 != c_values.end(); ++pv1) {
            bool consider_next_pv1 = false;

            for (auto pv2 = pv1; pv2 != c_values.end(); ++pv2) {
                assert(!consider_next_pv1);

                if (*pv2 == *pv1) continue; // we need a pair a different values
                // *pv1,*pv2 is the candidate pair

                if (hidden_pair(c, *pv1, *pv2, row(c), consider_next_pv1)) { acted_on_hidden_pair = true; break; }
                if (consider_next_pv1) { assert(!acted_on_hidden_pair); break; }

                if (hidden_pair(c, *pv1, *pv2, column(c), consider_next_pv1)) { acted_on_hidden_pair = true; break; }
                if (consider_next_pv1) { assert(!acted_on_hidden_pair); break; }

                if (hidden_pair(c, *pv1, *pv2, nonet(c), consider_next_pv1)) { acted_on_hidden_pair = true; break; }
                if (consider_next_pv1) { assert(!acted_on_hidden_pair); break; }
            }
            if (acted_on_hidden_pair) { break; }
        }
        if (acted_on_hidden_pair) break;
    }
    return acted_on_hidden_pair;
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
