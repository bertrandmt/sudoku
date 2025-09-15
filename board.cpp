// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "verbose.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

void Board::record_entry_form1(const std::string &entry) {
    if (entry.size() != 3) throw std::runtime_error("cannot parse entry");


    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) throw std::runtime_error("unset value");

    if (set_value_at(row, col, val)) throw std::runtime_error("did not succeed in setting entry");
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
    if (entries.size() != width * height) throw std::runtime_error("not the right number of entries");

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
            set_value_at(c.coord(), static_cast<Value>(entries[index] - '0'));
            break;

        default: // don't know what to do with this
            throw std::runtime_error("bad character in entry");
        }
        index++;
    }
}

Board::Board(Analyzer &analyzer, const std::string &board_desc)
    : mNoteCellsCount(width * height)
    , mNotesCount(mNoteCellsCount * kNine)
    , mAnalyzer(analyzer) {
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
        throw std::runtime_error("don't know how to parse this");
    }
}

Board::Board(Analyzer &analyzer, const Board &other)
    : mCells(other.mCells)
    , mNoteCellsCount(other.mNoteCellsCount)
    , mNotesCount(other.mNotesCount)
    , mAnalyzer(analyzer) {

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
