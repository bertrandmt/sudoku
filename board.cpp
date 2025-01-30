// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "verbose.h"

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
    , mLockedCandidates(other.mLockedCandidates)
    , mNakedPairs(other.mNakedPairs)
    , mHiddenPairs(other.mHiddenPairs) {

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
    size_t cnt = 1;
    for (auto const &c : mCells) {
        if (c.isValue()) out << c.value();
        else             out << '.';
        if (cnt++ % 9 == 0) out <<  " ";
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

            if (sVerbose) std::cout << "  [ANn] " << cell.coord() << " x" << other_cell.value()
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

            if (sVerbose) std::cout << "  [ANv] " << other_cell.coord() << " x" << cell.value()
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
    mNakedSingles.erase(std::remove_if(mNakedSingles.begin(), mNakedSingles.end(),
                [this](const auto &coord) {
                auto const &cell(this->at(coord));
                bool remove_me = cell.isValue() || cell.notes().count() == 0;
                if (remove_me) {
                    if (sVerbose) std::cout << "  [xNS] " << cell.coord() << std::endl;
                }
                return remove_me; }), mNakedSingles.end());

    for (auto const &cell : set) {
        if (cell.isValue()) continue; // only considering note cells
        if (cell.notes().count() != 1) continue; // this is the naked single rule: notes have only one entry

        if (std::find(mNakedSingles.begin(), mNakedSingles.end(), cell.coord()) == mNakedSingles.end()) {
            mNakedSingles.push_back(cell.coord());
            if (sVerbose) std::cout << "  [fNS] " << cell.coord() << std::endl;
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
        if (cell.isValue()) continue;           // only considering note cells
        if (cell.notes().count() == 1) {        // if there's only one note, the single is not really "hidden"
            auto it = std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(), [cell](auto const &entry) { return cell.coord() == entry.coord;});
            if (it != mHiddenSingles.end()) {   // if it was previously recorded as "hidden", remove it
                mHiddenSingles.erase(it);
                if (sVerbose) std::cout << "  [xHS] " << cell.coord() << " [NS]" << std::endl;
            }
            continue;                           // also, don't continue processing
        }

        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell

            std::string tag;
            if (test_hidden_single(cell, value, nonet(cell), tag)
             || test_hidden_single(cell, value, column(cell), tag)
             || test_hidden_single(cell, value, row(cell), tag)) {
                if (std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(), [cell](const auto &entry) { return cell.coord() == entry.coord; }) == mHiddenSingles.end()) {
                    HiddenSingle hs(cell.coord(), value, tag);
                    mHiddenSingles.push_back(hs);
                    if (sVerbose) std::cout << "  [fHS] " << hs << std::endl;
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
bool Board::test_locked_candidate(const Cell &cell, const Value &value, Set1 &set_to_consider, Set2 &set_to_ignore) {
    if (std::find_if(mLockedCandidates.begin(), mLockedCandidates.end(),
                [cell, value, set_to_ignore](const auto &entry) { return entry.contains(cell.coord(), value, set_to_ignore.tag()); }) != mLockedCandidates.end()) {
        // we've already record the entry containing this cell, for this set and value
        return false;
    }

    std::vector<Coord> lc_coords;
    lc_coords.push_back(cell.coord());

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
        LockedCandidates lc(lc_coords, value, set_to_ignore.tag());
        mLockedCandidates.push_back(lc);
        if (sVerbose) std::cout << "  [fLC] " << lc << std::endl;
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

    for (auto const &cell : set) {
        if (cell.isValue()) continue; // only considering note cells

        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell

            // form 1
            test_locked_candidate(cell, value, row(cell), nonet(cell));
            test_locked_candidate(cell, value, column(cell), nonet(cell));

            // form 2
            test_locked_candidate(cell, value, nonet(cell), row(cell));
            test_locked_candidate(cell, value, nonet(cell), column(cell));
        }
    }
}

void Board::find_locked_candidates(const Cell &) {
    find_locked_candidates();
}

void Board::find_locked_candidates() {
    mLockedCandidates.erase(mLockedCandidates.begin(), mLockedCandidates.end());
    find_locked_candidates_in_set(mCells);
}

template<class Set>
void Board::find_naked_pair(const Cell &cell, const Set &set) {
    assert(cell.isNote());
    auto c_values = cell.notes().values();
    assert(c_values.size() == 2);

    if (std::find_if(mNakedPairs.begin(), mNakedPairs.end(),
                [cell](auto const &entry) { return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; })
            != mNakedPairs.end()) return; // we've already recorded this pair

    auto v1 = c_values[0];
    auto v2 = c_values[1];

    for (auto const &pair_cell : set) {
        if (pair_cell.isValue()) continue;              // only considering note cells
        if (pair_cell == cell) continue;                // not considering this cell
        auto pair_cell_values = pair_cell.notes().values();
        if (pair_cell_values.size() != 2) continue;     // only considering other cells with two possible values in notes

        auto pc_v1 = pair_cell_values.at(0);
        auto pc_v2 = pair_cell_values.at(1);
        if (!((v1 == pc_v1 && v2 == pc_v2) || (v1 == pc_v2 && v2 == pc_v1))) continue; // it's a pair, but not the same pair

        // ok, it's a pair, but would acting on it have an effect?
        bool would_act = false;
        for (auto const &other_cell : set) {
            if (other_cell.isValue()) continue;
            if (other_cell == pair_cell || other_cell == cell) continue;
            if (!other_cell.check(v1) && !other_cell.check(v2)) continue; // no impact on this cell

            would_act = true;
        }
        if (would_act) {
            NakedPair np(std::make_pair(cell.coord(), pair_cell.coord()), std::make_pair(v1, v2));
            mNakedPairs.push_back(np);
            if (sVerbose) std::cout << "  [fNP] " << np << std::endl;
            break;
        }
    }
}

template<class Set>
void Board::find_naked_pairs_in_set(const Set &set) {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row,
    // or column, and no other candidates are possible in those cells, then those n candidates
    // are not possible elsewhere in that same block, row, or column.
    // Applied for n = 2

    for (auto const &cell : set) {
        if (cell.isValue()) continue; // only considering note cells
        auto c_values = cell.notes().values();
        if (c_values.size() != 2) continue; // only considering candidates with two possible values in notes

        find_naked_pair(cell, nonet(cell));
        find_naked_pair(cell, column(cell));
        find_naked_pair(cell, row(cell));
    }
}

void Board::find_naked_pairs(const Cell &) {
    find_naked_pairs();
}

void Board::find_naked_pairs() {
    mNakedPairs.erase(mNakedPairs.begin(), mNakedPairs.end());
    find_naked_pairs_in_set(mCells);
}

bool Board::test_hidden_pair(const HiddenPair &entry) {
    auto const &c1 = at(entry.coords.first);
    auto const &c2 = at(entry.coords.second);

    if (c1.isValue()) return false;
    if (c2.isValue()) return false;

    if (c1.notes().count() <= 2 && c2.notes().count() <= 2) return false; // the pair may not have a pair of candidates
                                                                          // or may not be hidden any longer
    if (!c1.check(entry.values.first) || !c1.check(entry.values.second))  return false;
    if (!c2.check(entry.values.first) || !c2.check(entry.values.second))  return false;

    // both cells in the entry are still notes with one of them having strictly more than two candidates
    // and they both have both entry values as candidates

    return true;
}

template<class Set>
void Board::test_hidden_pairs_in_set(const Set &set) {
    if (mHiddenPairs.size() != 0) {
        // all cells in the set have potentially been changed; for each, see
        // if it is in one of the hidden pair records, and if so re-validate that it
        // is still a hidden pair
        for (auto const &cell : set) {
            auto it = std::find_if(mHiddenPairs.begin(), mHiddenPairs.end(),
                    [cell](auto const &entry){ return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; });
            if (it != mHiddenPairs.end()) {
                if (!test_hidden_pair(*it)) {
                    if (sVerbose) std::cout << "  [xHP] " << *it << std::endl;
                    mHiddenPairs.erase(it);
                }
            }
        }
    }
}

template<class Set>
void Board::test_hidden_pair(const Cell &cell, const Value &v1, const Value &v2, const Set &set) {
    assert(cell.isNote());
    assert(cell.notes().check(v1));
    assert(cell.notes().check(v2));

    if (std::find_if(mHiddenPairs.begin(), mHiddenPairs.end(),
                [cell](auto const &entry) { return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; })
            != mHiddenPairs.end()) return; // we've already recorded this pair

    // can we find another note cell with the same pair in the same set, but no other cell with either candidate in the set?
    Cell *ppair_cell = NULL; // "the" other potential candidate
    bool condition_met = true;

    for (auto &other_cell : set) {
        if (other_cell.isValue()) continue;                                                // only considering note cells
        if (other_cell == cell) continue;                                                  // not considering this cell
        if (!other_cell.check(v1) && !other_cell.check(v2)) continue;                      // no impact on algorithm; check next cell in row
        if (other_cell.check(v1) ^ other_cell.check(v2)) { condition_met = false; break; } // either v1 or v2 is disqualified
        if (other_cell.notes().check(v1) && other_cell.notes().check(v2)) {
            if (!ppair_cell) {              // no candidate yet
                ppair_cell = &other_cell;   // this is "the" other candidate
                continue;
            }
            else {                          // this is disqualifying: we have more than two candidates in the row
                condition_met = false;
                break;
            }
        }
    }
    if (!ppair_cell) condition_met = false;                      // we did not, in fact, find another candidate
    if (cell.notes().count() == 2
     && (ppair_cell && ppair_cell->notes().count() == 2)) condition_met = false; // condition was met, in a way, but there is no action to take

    if (!condition_met) return;                                  // we're done here

    HiddenPair hp(std::make_pair(cell.coord(), ppair_cell->coord()), std::make_pair(v1, v2));
    mHiddenPairs.push_back(hp);
    if (sVerbose) std::cout << "  [fHP] " << hp << std::endl;
}

template<class Set>
void Board::find_hidden_pairs_in_set(const Set &set) {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row, or column,
    // and those n candidates are not possible elsewhere in that same block, row, or column, then no other
    // candidates are possible in those cells.
    // Applied for n = 2

    for (auto &c : set) {
        if (c.isValue())         continue; // only considering note cells
        auto c_values = c.notes().values();
        if (c_values.size() < 2) continue; // cannot possibly be hidden pair candidate

        for (auto pv1 = c_values.begin(); pv1 != c_values.end(); ++pv1) {
            for (auto pv2 = pv1 + 1; pv2 != c_values.end(); ++pv2) {

                assert(*pv2 != *pv1);
                // *pv1,*pv2 is the candidate pair

                test_hidden_pair(c, *pv1, *pv2, nonet(c));
                test_hidden_pair(c, *pv1, *pv2, column(c));
                test_hidden_pair(c, *pv1, *pv2, row(c));
            }
        }
    }
}

void Board::find_hidden_pairs(const Cell &cell) {
    // step 1: cell was changed; for all cells in its row, column and nonet
    // revalidate that, should it have been a member of a hidden pair, the
    // hidden pair is still valid
    test_hidden_pairs_in_set(nonet(cell));
    test_hidden_pairs_in_set(column(cell));
    test_hidden_pairs_in_set(row(cell));

    find_hidden_pairs_in_set(nonet(cell));
    find_hidden_pairs_in_set(column(cell));
    find_hidden_pairs_in_set(row(cell));
}

void Board::find_hidden_pairs() {
    find_hidden_pairs_in_set(mCells);
}

void Board::analyze(const Cell &cell) {
    find_naked_singles(cell);
    find_hidden_singles(cell);
    find_locked_candidates(cell);
    find_naked_pairs(cell);
    find_hidden_pairs(cell);
}

void Board::analyze() {
    find_naked_singles();
    find_hidden_singles();
    find_locked_candidates();
    find_naked_pairs();
    find_hidden_pairs();
}

bool Board::act_on_naked_single() {
    if (mNakedSingles.empty()) { return false; }

    auto const &coord = mNakedSingles.back();
    auto &cell = at(coord);

    std::vector<Value> values = cell.notes().values();
    assert(values.size() == 1);
    Value value = values.at(0);

    std::cout << "[NS] " << cell.coord() << " =" << value << std::endl;
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

    std::cout << "[HS] " << cell.coord() << " =" << entry.value << " [" << entry.tag << "]" << std::endl;
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
            other_cell.set(entry.values.first, false);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.first << " [" << set.tag() << "]" << std::endl;
            acted_on_other_cell = true;
        }
        if (other_cell.check(entry.values.second)) {
            other_cell.set(entry.values.second, false);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.second << " [" << set.tag() << "]" << std::endl;
            acted_on_other_cell = true;
        }

        if (acted_on_other_cell) {
            autonote(other_cell);
            analyze(other_cell);
            acted_on_naked_pair = true;
        }
    }

    return acted_on_naked_pair;
}

bool Board::act_on_naked_pair() {
    bool acted_on_naked_pair = false;

    if (mNakedPairs.empty()) { return acted_on_naked_pair; }

    auto const entry = mNakedPairs.back(); // copy
    auto const &cell1 = at(entry.coords.first);

    acted_on_naked_pair |= act_on_naked_pair(entry, nonet(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, column(cell1));
    acted_on_naked_pair |= act_on_naked_pair(entry, row(cell1));

    return acted_on_naked_pair;
}

bool Board::act_on_hidden_pair(Cell &cell, const HiddenPair &entry) {
    bool acted_on_hidden_pair = false;

    auto const &v1 = entry.values.first;
    auto const &v2 = entry.values.second;

    for (auto const &value : cell.notes().values()) {
        if (value == v1) continue;
        if (value == v2) continue;

        cell.set(value, false);
        std::cout << "[HP] " << cell.coord() << " x" << value << " " << entry << std::endl;
        acted_on_hidden_pair = true;
    }

    return acted_on_hidden_pair;
}

bool Board::act_on_hidden_pair() {
    bool acted_on_hidden_pair = false;

    if (mHiddenPairs.empty()) { return acted_on_hidden_pair; }

    auto const &entry = mHiddenPairs.back();
    auto &c1 = at(entry.coords.first);
    auto &c2 = at(entry.coords.second);

    acted_on_hidden_pair |= act_on_hidden_pair(c1, entry);
    acted_on_hidden_pair |= act_on_hidden_pair(c2, entry);

    mHiddenPairs.pop_back();

    if (acted_on_hidden_pair) {
        autonote(c1);
        autonote(c2);
        analyze(c1);
        analyze(c2);
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
