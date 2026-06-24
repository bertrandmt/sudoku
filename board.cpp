// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "verbose.h"

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {
// Reject a board where any digit repeats within a unit (row, column, or
// nonet). The analyzers assume a logically consistent board; enforcing that
// at load time keeps a contradictory board from ever reaching them.
template <typename Subsets>
void reject_duplicate_values(Subsets &subsets, const char *unit_kind) {
    for (auto &subset : subsets) {
        std::array<bool, static_cast<size_t>(kNine)> seen{};
        for (auto &cell : subset) {
            if (!cell.isValue()) continue;
            size_t i = static_cast<size_t>(cell.value()) - 1;   // 1..9 -> 0..8
            if (seen[i]) {
                throw std::runtime_error(
                    std::string("invalid board: value ") + std::to_string(cell.value())
                    + " appears more than once in a " + unit_kind);
            }
            seen[i] = true;
        }
    }
}
} // namespace

bool parse_rcv(const std::string &entry, size_t &row, size_t &col, Value &val) {
    if (entry.size() != 3) return false;

    if (entry[0] < '1' || entry[0] > '9') return false;
    if (entry[1] < '1' || entry[1] > '9') return false;
    if (entry[2] < '1' || entry[2] > '9') return false;

    row = entry[0] - '1';
    col = entry[1] - '1';
    val = static_cast<Value>(entry[2] - '0');
    return true;
}

void Board::record_entry_form1(const std::string &entry) {
    size_t row, col;
    Value val;
    if (!parse_rcv(entry, row, col, val))
        throw std::runtime_error("cannot parse entry \"" + entry + "\": expected 3 digits 1-9 (row, column, value)");

    if (!set_value_at(row, col, val))
        throw std::runtime_error("cell (" + std::to_string(row + 1) + "," + std::to_string(col + 1) + ") set more than once");
}

void Board::record_entries_form1(const std::string &entries) {
    size_t ofsb = 0, ofse = 0;
    while (ofse != std::string::npos) {
        ofse = entries.find(';', ofsb);
        std::string entry = entries.substr(ofsb, ofse - ofsb);
        record_entry_form1(entry);
        ofsb = ofse + 1;
    }
}

void Board::record_entries_form2(const std::string &entries) {
    if (entries.size() != width * height)
        throw std::runtime_error("expected " + std::to_string(width * height) + " cells, got " + std::to_string(entries.size()));

    size_t index = 0;
    for (auto &c : mCells) {
        switch (entries[index]) {
        case '0':
        case '.': // it's a note entry
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': // it's a value entry
            if (!set_value_at(c.coord(), static_cast<Value>(entries[index] - '0'))) {
                throw std::runtime_error("cell (" + std::to_string(c.coord().row() + 1) + "," + std::to_string(c.coord().column() + 1) + ") set more than once");
            }
            break;

        default: // don't know what to do with this
            throw std::runtime_error(std::string("invalid character '") + entries[index] + "' at position " + std::to_string(index + 1) + " (use digits 1-9 or '.')");
        }
        index++;
    }
}

Board::Board(const std::string &board_desc)
    : mNoteCellsCount(width * height)
    , mNotesCount(mNoteCellsCount * kNine) {
    for (size_t row = 0; row < height; row++) {
        for (size_t col = 0; col < width; col++) {
            mCells.push_back(Cell(row, col));
        }
    }
    rebuild_subsets();

    switch(board_desc[0]) {
    case ';':
        record_entries_form1(board_desc.substr(1));
        break;

    case '.':
        record_entries_form2(board_desc.substr(1));
        break;

    default:
        if (board_desc.empty()) throw std::runtime_error("no board provided");
        throw std::runtime_error("board must start with ';' (row,column,value form) or '.' (81-cell form)");
    }

    reject_duplicate_values(mRows,    "row");
    reject_duplicate_values(mColumns, "column");
    reject_duplicate_values(mNonets,  "nonet");
}

Board::Board(const Board &other)
    : mCells(other.mCells)
    , mNoteCellsCount(other.mNoteCellsCount)
    , mNotesCount(other.mNotesCount) {

    rebuild_subsets();
}

void Board::rebuild_subsets() {
    assert(mCells.size() == width * height);

    mRows.clear();
    mColumns.clear();
    mNonets.clear();

    for (size_t row = 0; row < height; row++) {
        Row r(*this, row);
        mRows.push_back(r);
    }
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

void Board::print(std::ostream &out) const {
    size_t cnt = 1;
    for (auto const &c : mCells) {
        if (c.isValue()) out << c.value();
        else             out << '.';
        if (cnt++ % 9 == 0) out <<  " ";
    }
    out << std::endl;
}

bool Board::clear_note_at(size_t row, size_t col, const Value &value) {
    return clear_note_at(Coord(row, col), value);
}

bool Board::clear_note_at(const Coord &coord, const Value &value) {
    auto &cell = at(coord);

    if (!cell.isNote()) return false;
    if (!cell.check(value)) return false;

    cell.set(value, false);
    mNotesCount--;

    return true;
}

bool Board::set_value_at(size_t row, size_t col, const Value &value) {
    return set_value_at(Coord(row, col), value);
}

bool Board::set_value_at(const Coord &coord, const Value &value) {
    auto &cell = at(coord);

    if (!cell.isNote()) return false;

    mNotesCount -= cell.notes().count();
    mNoteCellsCount--;
    cell.set(value);

    return true;
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
    return row(c.coord());
}

const Column &Board::column(const Cell &c) const {
    return column(c.coord());
}

const Nonet &Board::nonet(const Cell &c) const {
    return nonet(c.coord());
}

const Row &Board::row(const Coord &coord) const {
    return mRows.at(coord.row());
}

const Column &Board::column(const Coord &coord) const {
    return mColumns.at(coord.column());
}

const Nonet &Board::nonet(const Coord &coord) const {
    size_t nonetRow = (coord.row() / 3) * 3;
    size_t nonetCol = (coord.column() / 3) * 3;

    return mNonets.at(nonetRow + nonetCol / 3);
}

bool Board::see_each_other(const Coord &coord1, const Coord &coord2, std::string &out_tag) const {
    bool did_see_each_other = false;

    if      (row(coord1)    == row(coord2))    { did_see_each_other = true; out_tag.append("r"); }
    else if (column(coord1) == column(coord2)) { did_see_each_other = true; out_tag.append("c"); }
    else if (nonet(coord1)  == nonet(coord2))  { did_see_each_other = true; out_tag.append("n"); }

    return did_see_each_other;
}

bool Board::any_see_each_other(const std::vector<Coord> &coords, std::string &out_tag) const {
    bool did_any_see_each_other = false;

    for (size_t i = 0; i < coords.size(); ++i) {
        for (size_t j = i + 1; j < coords.size(); ++j) {
            if (see_each_other(coords[i], coords[j], out_tag)) {
                did_any_see_each_other = true;
                break;
            }
        }
        if (did_any_see_each_other) break;
    }

    return did_any_see_each_other;
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
    if (b.mNoteCellsCount != 0) {
        outs << std::endl
             << "Left to solve:   " << b.mNoteCellsCount << std::endl
             << "Notes remaining: " << b.mNotesCount;
    }

    return outs;
}
