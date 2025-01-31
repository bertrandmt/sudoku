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
#include <format>

void Board::record_entry_form1(const std::string &entry) {
    if (entry.size() != 3) throw std::format_error("cannot parse entry");


    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) throw std::format_error("unset value");

    if (set_value_at(row, col, val)) throw std::format_error("did not succeed in setting entry");
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
    if (entries.size() != width * height) throw std::format_error("not the right number of entries");

    size_t index = 0;
    for (auto &c : mCells) {
        switch (entries[index]) {
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
            throw std::format_error("bad character in entry");
        }
        index++;
    }
}

Board::Board(const std::string &board_desc) {
    for (size_t row = 0; row < height; row++) {
        for (size_t col = 0; col < width; col++) {
            mCells.push_back(Cell(row, col));
        }
    }
    rebuild_subsets();

    mAnalyzer.reset(new Analyzer(*this));

    switch(board_desc[0]) {
    case ';':
        record_entries_form1(board_desc.substr(1));
        break;

    case '.':
        record_entries_form2(board_desc.substr(1));
        break;

    default:
        throw std::format_error("don't know how to parse this");
    }

    mAnalyzer->analyze();
}

Board::Board(const Board &other)
    : mCells(other.mCells)
    , mNakedSingles(other.mNakedSingles)
    , mNakedPairs(other.mNakedPairs)
    , mHiddenSingles(other.mHiddenSingles)
    , mLockedCandidates(other.mLockedCandidates)
    , mHiddenPairs(other.mHiddenPairs) {

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

    mAnalyzer->notes_dirty(cell);

    return true;
}

bool Board::set_value_at(size_t row, size_t col, const Value &value) {
    return set_value_at(Coord(row, col), value);
}

bool Board::set_value_at(const Coord &coord, const Value &value) {
    auto &cell = at(coord);

    if (!cell.isNote()) return false;

    cell.set(value);

    mAnalyzer->value_dirty(cell);

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

bool Board::act(const bool singles_only) {
    bool did_act = false;

    mAnalyzer.reset(new Analyzer(*this));

    did_act = act_on_naked_single();
    if (!did_act) did_act = act_on_naked_pair();
    if (!did_act) did_act = act_on_hidden_single();

    if (!singles_only) {
        if (!did_act) did_act = act_on_locked_candidate();
        if (!did_act) did_act = act_on_hidden_pair();
    }

    mAnalyzer->analyze();

    return did_act;
}

bool Board::act_on_naked_single() {
    if (mNakedSingles.empty()) { return false; }

    auto const coord = mNakedSingles.back();
    mNakedSingles.pop_back();

    auto &cell = at(coord);

    std::vector<Value> values = cell.notes().values();
    assert(values.size() == 1);
    Value value = values.at(0);

    std::cout << "[NS] " << cell.coord() << " =" << value << std::endl;
    set_value_at(coord, value);

    return true;
}

bool Board::act_on_hidden_single() {
    if (mHiddenSingles.empty()) { return false; }

    auto const entry = mHiddenSingles.back();
    mHiddenSingles.pop_back();

    std::cout << "[HS] " << entry.coord << " =" << entry.value << " [" << entry.tag << "]" << std::endl;
    set_value_at(entry.coord, entry.value);

    return true;
}

template<class Set>
bool Board::act_on_naked_pair(const NakedPair &entry, Set &set) {
    bool acted_on_naked_pair = false;

    auto const &cell1 = at(entry.coords.first);
    auto const &cell2 = at(entry.coords.second);

    if (!set.contains(cell2)) return acted_on_naked_pair; // this is not the set to act on

    for (auto &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == cell1 || other_cell == cell2) continue; // not looking at either of the cell pairs

        bool acted_on_other_cell = false;
        if (other_cell.check(entry.values.first)) {
            clear_note_at(other_cell.coord(), entry.values.first);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.first << " [" << set.tag() << "]" << std::endl;
            acted_on_other_cell = true;
        }
        if (other_cell.check(entry.values.second)) {
            clear_note_at(other_cell.coord(), entry.values.second);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.second << " [" << set.tag() << "]" << std::endl;
            acted_on_other_cell = true;
        }

        if (acted_on_other_cell) {
            acted_on_naked_pair = true;
        }
    }

    return acted_on_naked_pair;
}

bool Board::act_on_naked_pair() {
    bool acted_on_naked_pair = false;

    if (mNakedPairs.empty()) { return acted_on_naked_pair; }

    auto const entry = mNakedPairs.back();
    mNakedPairs.pop_back();
    auto const &cell1 = at(entry.coords.first);

    acted_on_naked_pair |= act_on_naked_pair(entry, nonet(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, column(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, row(cell1));

    return acted_on_naked_pair;
}

template<class Set>
bool Board::act_on_locked_candidate(const LockedCandidates &entry, Set &set) {
    bool acted_on_locked_candidates = false;

    for (auto &other_cell : set) {
        // is this a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it one of the locked candidate?
        if (std::find(entry.coords.begin(), entry.coords.end(), other_cell.coord())
                != entry.coords.end()) continue;

        // no! but is it a candidate for the locked value?
        if (!other_cell.check(entry.value)) continue;

        // yes! we'll act
        std::cout << "[LC] " << other_cell.coord() << " x" << entry.value << " [" << entry.tag << "]" << std::endl;
        clear_note_at(other_cell.coord(), entry.value);
        acted_on_locked_candidates = true;
    }

    //assert(acted_on_locked_candidates);
    return acted_on_locked_candidates;
}

bool Board::act_on_locked_candidate() {
    if (mLockedCandidates.empty()) { return false; }

    auto const entry = mLockedCandidates.back(); // copy
    mLockedCandidates.pop_back();

    auto &cell = at(entry.coords.at(0));

    switch (entry.tag[0]) {
    case 'n':
        (void) act_on_locked_candidate(entry, nonet(cell));
        break;
    case 'c':
        (void) act_on_locked_candidate(entry, column(cell));
        break;
    case 'r':
        (void) act_on_locked_candidate(entry, row(cell));
        break;
    }

    return true;
}

bool Board::act_on_hidden_pair(Cell &cell, const HiddenPair &entry) {
    bool acted_on_hidden_pair = false;

    auto const &v1 = entry.values.first;
    auto const &v2 = entry.values.second;

    for (auto const &value : cell.notes().values()) {
        if (value == v1) continue;
        if (value == v2) continue;

        clear_note_at(cell.coord(), value);
        std::cout << "[HP] " << cell.coord() << " x" << value << " " << entry << std::endl;
        acted_on_hidden_pair = true;
    }

    return acted_on_hidden_pair;
}

bool Board::act_on_hidden_pair() {
    bool acted_on_hidden_pair = false;

    if (mHiddenPairs.empty()) { return acted_on_hidden_pair; }

    auto const entry = mHiddenPairs.back();
    mHiddenPairs.pop_back();

    auto &c1 = at(entry.coords.first);
    auto &c2 = at(entry.coords.second);

    acted_on_hidden_pair |= act_on_hidden_pair(c1, entry);
    acted_on_hidden_pair |= act_on_hidden_pair(c2, entry);

    return acted_on_hidden_pair;
}

std::ostream& operator<<(std::ostream& outs, const Board &b) {
    // the board itself
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
    outs << "+=====+=====+=====++=====+=====+=====++=====+=====+=====+" << std::endl
    // naked singles
         << "[NS](" << b.mNakedSingles.size() << ") {";
    bool is_first = true;
    for (auto const &coord : b.mNakedSingles) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << coord;
    }
    outs << "}" << std::endl
   // hidden singles
         << "[HS](" << b.mHiddenSingles.size() << ") {";
    is_first = true;
    for (auto const &entry: b.mHiddenSingles) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << entry;
    }
    outs << "}" << std::endl
    // naked pairs
         << "[NP](" << b.mNakedPairs.size() << ") {";
    is_first = true;
    for (auto const &entry: b.mNakedPairs) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}" << std::endl
     // locked candidates
         << "[LC](" << b.mLockedCandidates.size() << ") {";
    is_first = true;
    for (auto const &entry: b.mLockedCandidates) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}" << std::endl
    // hidden pairs
         << "[HP](" << b.mHiddenPairs.size() << ") {";
    is_first = true;
    for (auto const &entry: b.mHiddenPairs) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}";

    return outs;
}

std::ostream& operator<<(std::ostream& outs, const Board::HiddenSingle &hs) {
    return outs << hs.coord << "#" << hs.value << "[" << hs.tag << "]";
}

std::ostream& operator<<(std::ostream& outs, const Board::LockedCandidates &lc) {
    outs << "{";
    bool is_first = true;
    for (auto const &coord : lc.coords) {
        if (!is_first) { std::cout << ","; }
        is_first = false;
        outs << coord;
    }
    outs << "}#" << lc.value << "[^" << lc.tag << "]";
    return outs;
}

std::ostream& operator<<(std::ostream& outs, const Board::NakedPair &np) {
    return outs << "{" << np.coords.first << "," << np.coords.second << "}#{" << np.values.first << "," << np.values.second << "}";
}

std::ostream& operator<<(std::ostream& outs, const Board::HiddenPair &hp) {
    return outs << "{" << hp.coords.first << "," << hp.coords.second << "}#{" << hp.values.first << "," << hp.values.second << "}";
}
