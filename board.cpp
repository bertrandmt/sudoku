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
            mCells.push_back(Cell(row, col));
        }
    }
    rebuild_subsets();
}

Board::Board(const Board &other)
    : mCells(other.mCells)
    , mNakedSingles(other.mNakedSingles)
    , mHiddenSingles(other.mHiddenSingles)
    , mLockedCandidates(other.mLockedCandidates) {

    rebuild_subsets();
}

void Board::rebuild_subsets() {
    assert(mCells.size() == kSUDOKU_BOARD_SIZE);

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

void Board::print(std::ostream &out) {
    for (auto const &c : mCells) {
        if (c.isValue()) out << c.value();
        else             out << '.';
    }
    out << std::endl;
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

            std::cout << "  [ANn] " << cell.coord() << " x" << other_cell.value()
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

            std::cout << "  [ANv] " << other_cell.coord() << " x" << cell.value()
                      << " " << set.tag() << "(" << cell.coord() << ")" << std::endl;
            other_cell.notes().set(cell.value(), false);
        }
    }
}

void Board::autonote(Cell &cell) {
    autonote(cell, nonet(cell));
    autonote(cell, column(cell));
    autonote(cell, row(cell));
}

void Board::autonote() {
    for (auto &cell : mCells) {
        autonote(cell);
    }
    analyze();
}

template<class Set>
void Board::find_naked_singles_in_set(const Set &set) {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A naked single arises when there is only one possible candidate for a cell
    for (auto const &cell : set) {
        if (cell.isValue()) continue; // only considering note cells
        if (cell.notes().count() != 1) continue; // this is the naked single rule: notes have only one entry

        if (std::find(mNakedSingles.begin(), mNakedSingles.end(), cell.coord()) == mNakedSingles.end()) {
            mNakedSingles.push_back(cell.coord());
            std::cout << "  [fNS] " << cell.coord() << std::endl;
        }
    }
}

void Board::find_naked_singles(const Cell &cell) {
    if (cell.isValue()) {
        const auto &it = std::find(mNakedSingles.begin(), mNakedSingles.end(), cell.coord());
        if (it != mNakedSingles.end()) {
            mNakedSingles.erase(it);
        }
    }

    find_naked_singles_in_set(nonet(cell));
    find_naked_singles_in_set(column(cell));
    find_naked_singles_in_set(row(cell));
}

void Board::find_naked_singles() {
    find_naked_singles_in_set(mCells);
}

template<class Set>
bool Board::test_hidden_single(const Cell &cell, const Value &value, const Set &set, std::string &tag) const {
    for (auto const &other_cell : set) {
        if (other_cell == cell) continue;               // do not consider the current cell
        assert(other_cell.isNote() || other_cell.value() != value);
        if (other_cell.isValue()) continue;             // only considering note cells
        if (!other_cell.notes().check(value)) continue; // this note cell is _not_ a candidate

        return false;
    }
    tag.append(set.tag());
    return true;
}

template<class Set>
void Board::find_hidden_singles_in_set(const Set &set) {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    for (auto &cell : set) {
        if (cell.isValue()) continue; // only considering note cells

        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell

            std::string tag;
            if (test_hidden_single(cell, value, nonet(cell), tag)
             || test_hidden_single(cell, value, column(cell), tag)
             || test_hidden_single(cell, value, row(cell), tag)) {
                if (std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(), [cell](const auto &entry) { return cell.coord() == entry.coord; }) == mHiddenSingles.end()) {
                    HiddenSingle hs(cell.coord(), value, tag);
                    mHiddenSingles.push_back(hs);
                    std::cout << "  [fHS] " << hs << std::endl;
                }
            }
        }
    }
}

void Board::find_hidden_singles(const Cell &cell) {
    if (cell.isValue()) {
        const auto &it = std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(), [cell](const auto &entry) { return cell.coord() == entry.coord; });
        if (it != mHiddenSingles.end()) {
            mHiddenSingles.erase(it);
        }
    }

    find_hidden_singles_in_set(nonet(cell));
    find_hidden_singles_in_set(column(cell));
    find_hidden_singles_in_set(row(cell));
}

void Board::find_hidden_singles() {
    find_hidden_singles_in_set(mCells);
}

template<class Set1, class Set2>
bool Board::test_locked_candidate(const Cell &cell, const Value &value, Set1 &set_to_consider, Set2 &set_to_ignore, std::vector<Coord> &lc_coords) {
    for (auto const &other_cell : set_to_consider) {
        if (other_cell.isValue()) continue;                                                             // do not consider value cells
        if (other_cell == cell) continue;                                                               // do not consider the current cell
        assert(other_cell.isNote() || other_cell.value() != value);
        if (!other_cell.check(value)) continue;                                                         // not a candidate
        if (std::find(set_to_ignore.begin(), set_to_ignore.end(), other_cell) != set_to_ignore.end()) { // this is another one of the candidates
            lc_coords.push_back(other_cell.coord());    // record it
            continue;                                   // and continue the search
        }

        return false;
    }

    // ensure that this set of locked candidates, if acted on, *would* have an effect
    for (auto const &other_cell : set_to_ignore) {
        if (other_cell.isValue()) continue;                                                                           // do not consider value cells
        if (other_cell == cell) continue;                                                                             // do not consider the current cell
        if (!other_cell.check(value)) continue;                                                                       // not a candidate
        if (std::find(set_to_consider.begin(), set_to_consider.end(), other_cell) != set_to_consider.end()) continue; // this is one of the locked candidaets

        // we found a note cell that is in the rest of the "set_to_ignore" and also is a candidate for this value:
        // we *would* act on it
        return true;
    }

    return false;
}

template<class Set>
void Board::find_locked_candidates_in_set(const Set &set) {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
    // Form 1:
    // When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same row/column,
    // then it is also not possible anywhere else in the same nonet
    // Form 2:
    // When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same nonet,
    // then it is also not possible anywhere else in the same row/column

    for (auto const &c : set) {
        if (c.isValue()) continue; // only considering note cells

        for (auto const &v : c.notes().values()) { // for each candidate value in this note cell

            // form 1
            std::vector<Coord> lc_coords(1, c.coord());
            if (std::find_if(mLockedCandidates.begin(), mLockedCandidates.end(), [c,v](const auto &entry) { return entry.contains(c.coord(), v, "n"); }) == mLockedCandidates.end()
             && test_locked_candidate(c, v, row(c), nonet(c), lc_coords)) {
                LockedCandidates lc(lc_coords, v, "n");
                mLockedCandidates.push_back(lc);
                std::cout << "  [fLC] " << lc << std::endl;
            }

            lc_coords.erase(lc_coords.begin() + 1, lc_coords.end());
            if (std::find_if(mLockedCandidates.begin(), mLockedCandidates.end(), [c,v](const auto &entry) { return entry.contains(c.coord(), v, "n"); }) == mLockedCandidates.end()
             && test_locked_candidate(c, v, column(c), nonet(c), lc_coords)) {
                LockedCandidates lc(lc_coords, v, "n");
                mLockedCandidates.push_back(lc);
                std::cout << "  [fLC] " << lc << std::endl;
            }

            // form 2
            lc_coords.erase(lc_coords.begin() + 1, lc_coords.end());
            if (std::find_if(mLockedCandidates.begin(), mLockedCandidates.end(), [c,v](const auto &entry) { return entry.contains(c.coord(), v, "r"); }) == mLockedCandidates.end()
             && test_locked_candidate(c, v, nonet(c), row(c), lc_coords)) {
                LockedCandidates lc(lc_coords, v, "r");
                mLockedCandidates.push_back(lc);
                std::cout << "  [fLC] " << lc << std::endl;
            }

            lc_coords.erase(lc_coords.begin() + 1, lc_coords.end());
            if (std::find_if(mLockedCandidates.begin(), mLockedCandidates.end(), [c,v](const auto &entry) { return entry.contains(c.coord(), v, "c"); }) == mLockedCandidates.end()
             && test_locked_candidate(c, v, nonet(c), column(c), lc_coords)) {
                LockedCandidates lc(lc_coords, v, "c");
                mLockedCandidates.push_back(lc);
                std::cout << "  [fLC] " << lc << std::endl;
            }
        }
    }
}

void Board::find_locked_candidates(const Cell &) {
    mLockedCandidates.erase(mLockedCandidates.begin(), mLockedCandidates.end());
    find_locked_candidates_in_set(mCells);
}

void Board::find_locked_candidates() {
    mLockedCandidates.erase(mLockedCandidates.begin(), mLockedCandidates.end());
    find_locked_candidates_in_set(mCells);
}


void Board::analyze(const Cell &cell) {
    find_naked_singles(cell);
    find_hidden_singles(cell);
    find_locked_candidates(cell);
}

void Board::analyze() {
    find_naked_singles();
    find_hidden_singles();
    find_locked_candidates();
}

bool Board::act_on_naked_single() {
    if (mNakedSingles.empty()) { return false; }

    auto const &coord = mNakedSingles.back();
    auto &cell = at(coord);

    std::vector<Value> values = cell.notes().values();
    assert(values.size() == 1);
    Value value = values.at(0);

    std::cout << "[NS] cell" << cell.coord() << " =" << value << std::endl;
    cell.set(value);

    mNakedSingles.pop_back();

    autonote(cell);
    analyze(cell);

    return true;
}

bool Board::act_on_hidden_single() {
    if (mHiddenSingles.empty()) { return false; }

    auto const &entry = mHiddenSingles.back();
    auto &cell = at(entry.coord);

    std::cout << "[HS] cell" << cell.coord() << " =" << entry.value << " [" << entry.tag << "]" << std::endl;
    cell.set(entry.value);

    mHiddenSingles.pop_back();

    autonote(cell);
    analyze(cell);

    return true;
}

template<class Set>
bool Board::act_on_locked_candidate(const LockedCandidates &entry, Set &set) {
    bool acted_on_locked_candidates = false;

    for (auto &other_cell : set) {
        if (other_cell.isValue()) continue;      // only considering note cells
        if (std::find(entry.coords.begin(), entry.coords.end(), other_cell.coord())
                != entry.coords.end()) continue; // this is one of the locked candidates
        if (!other_cell.check(entry.value)) continue;  // this note cell is _not_ a candidate for the same value

        std::cout << "[LC] " << other_cell.coord() << " x" << entry.value << " [" << entry.tag << "]" << std::endl;
        other_cell.set(entry.value, false);
        acted_on_locked_candidates = true;

        autonote(other_cell);
        analyze(other_cell);
    }

    assert(acted_on_locked_candidates);
    return acted_on_locked_candidates;
}

bool Board::act_on_locked_candidate() {
    if (mLockedCandidates.empty()) { return false; }

    auto const entry = mLockedCandidates.back(); // copy
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

    for (auto const &c : mCells) {
        if (c.isValue()) continue; // only considering note cells
        auto c_values = c.notes().values();
        assert(c_values.size() >= 2); // otherwise would have been caught at single stage.
        if (c_values.size() != 2) continue; // only considering candidates with two possible values in notes

        if (naked_pair(c, nonet(c))) { acted_on_naked_pair = true; break; }
        if (naked_pair(c, column(c))) { acted_on_naked_pair = true; break; }
        if (naked_pair(c, row(c))) { acted_on_naked_pair = true; break; }
    }

    if (acted_on_naked_pair) analyze();
    return acted_on_naked_pair;
}

template<class Set>
bool Board::hidden_pair(Cell &cell, const Value &v1, const Value &v2, Set &set) {
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
        if (other_cell.notes().check(v1) ^ other_cell.notes().check(v2)) { condition_met = false; break; } // either v1 or v2 is disqualified
        if (other_cell.notes().check(v1) && other_cell.notes().check(v2)) {
            if (!ppair_cell) { // no candidate yet
                ppair_cell = &other_cell; // this is "the" other candidate
                continue;
            }
            else { // this is disqualifying: we have more than two candidates in the row
                condition_met = false;
                break;
            }
        }
    }
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

    for (auto &c : mCells) {
        if (c.isValue()) continue; // only considering note cells
        auto c_values = c.notes().values();
        assert(c_values.size() >= 2); // otherwise would have been caught at single stage.

        for (auto pv1 = c_values.begin(); pv1 != c_values.end(); ++pv1) {

            for (auto pv2 = pv1; pv2 != c_values.end(); ++pv2) {

                if (*pv2 == *pv1) continue; // we need a pair a different values
                // *pv1,*pv2 is the candidate pair

                if (hidden_pair(c, *pv1, *pv2, nonet(c))) { acted_on_hidden_pair = true; break; }
                if (hidden_pair(c, *pv1, *pv2, column(c))) { acted_on_hidden_pair = true; break; }
                if (hidden_pair(c, *pv1, *pv2, row(c))) { acted_on_hidden_pair = true; break; }
            }
            if (acted_on_hidden_pair) { break; }
        }
        if (acted_on_hidden_pair) break;
    }
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
    // locked candidates
         << "[LC](" << b.mLockedCandidates.size() << ") {";
    is_first = true;
    for (auto const &entry: b.mLockedCandidates) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}";

    return outs;
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

std::ostream& operator<<(std::ostream& outs, const Board::HiddenSingle &hs) {
    return outs << hs.coord << "#" << hs.value << "[" << hs.tag << "]";
}
