// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "verbose.h"

#include <stdexcept>

void Analyzer::value_dirty(const Cell &cell) {
    mValueDirtySet.insert(cell.coord());
}

void Analyzer::notes_dirty(const Cell &cell) {
    mNotesDirtySet.insert(cell.coord());
}

template<class Set>
void Analyzer::filter_notes_for_set(Cell &cell, const Set &set) {
    if (cell.isNote()) {                                   // note cell; update its own notes from all the value cells in the same set
        for (auto const &other_cell : set) {
            if (other_cell.isNote()) continue;             // other note cells do not participate in this update
            if (!cell.check(other_cell.value())) continue; // the value of other_cell is already checked off cell's notes

            if (sVerbose) std::cout << "  [FNn] " << cell.coord() << " x" << other_cell.value()
                      << " " << set.tag() << "(" << other_cell.coord() << ")" << std::endl;
            mBoard->clear_note_at(cell.coord(), other_cell.value());
        }
    }
    else {                                                 // value cell; let's update notes in note cells in the same set
        assert(cell.isValue());
        for (auto &other_cell : set) {
            if (other_cell.isValue()) continue;            // other value cells do not get changed by this process
            if (!other_cell.check(cell.value())) continue; // the value of cell is already checked off other_cell's notes

            if (sVerbose) std::cout << "  [FNv] " << other_cell.coord() << " x" << cell.value()
                      << " " << set.tag() << "(" << cell.coord() << ")" << std::endl;
            mBoard->clear_note_at(other_cell.coord(), cell.value());
        }
    }
}

template<class Set>
void Analyzer::filter_notes(const Set &set) {
    for (auto &cell : set) {
        filter_notes_for_set(cell, mBoard->nonet(cell));
        filter_notes_for_set(cell, mBoard->column(cell));
        filter_notes_for_set(cell, mBoard->row(cell));
    }
}

void Analyzer::filter_notes() {
    for (auto const &coord : mValueDirtySet) { // for each value cell whose value was set since last analysis
//        if (sVerbose) std::cout << "  [FN] considering " << *it << std::endl;

        auto &cell = mBoard->at(coord);         // review the notes in
        filter_notes(mBoard->nonet(cell));      // each cell in this cell's nonet, and
        filter_notes(mBoard->column(cell));     //                      ... column, and
        filter_notes(mBoard->row(cell));        //                      ... row
    }
}

bool Analyzer::test_naked_single(const Cell &cell) const {
    return cell.isNote()
        && cell.notes().count() == 1;
}

void Analyzer::filter_naked_singles() const {
    mBoard->mNakedSingles.erase(std::remove_if(mBoard->mNakedSingles.begin(), mBoard->mNakedSingles.end(),
                [this](const auto &coord) {
                    auto &cell = mBoard->at(coord);
                    bool is_naked_single = test_naked_single(cell);
                    if (!is_naked_single) {
                        if (sVerbose) std::cout << "  [xNS] " << coord << std::endl;
                    }
                    return !is_naked_single;
                }), mBoard->mNakedSingles.end());
}

void Analyzer::find_naked_singles() const {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A naked single arises when there is only one possible candidate for a cell
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // is this a naked single?
        if (!test_naked_single(cell)) continue;

        // yes! but do we already know about it?
        if (std::find(mBoard->mNakedSingles.begin(), mBoard->mNakedSingles.end(), cell.coord()) != mBoard->mNakedSingles.end()) continue;

        // no! let's record it
        mBoard->mNakedSingles.push_back(cell.coord());
        if (sVerbose) std::cout << "  [fNS] " << cell.coord() << std::endl;
    }
}

template<class Set>
bool Analyzer::test_hidden_single(const Cell &cell, const Value &value, const Set &set, std::string &out_tag) const {
    if (!cell.isNote()) return false;
    if (cell.notes().count() <= 1) return false; // either a naked single, or an impossibility
    if (!cell.check(value)) return false;        // note a candidate for value any longer

    for (auto const &other_cell : set) {
        if (other_cell == cell) continue;               // do not consider the current cell
        assert(other_cell.isNote() || other_cell.value() != value);
        if (other_cell.isValue()) continue;             // only considering note cells
        if (!other_cell.notes().check(value)) continue; // this note cell is _not_ a candidate

        return false;
    }
    out_tag.append(set.tag());
    return true;
}

void Analyzer::filter_hidden_singles() const {
    mBoard->mHiddenSingles.erase(std::remove_if(mBoard->mHiddenSingles.begin(), mBoard->mHiddenSingles.end(),
                [this](const auto &entry) {
                    auto &cell = mBoard->at(entry.coord);
                    bool is_hidden_single = false;
                    std::string out_tag;
                    if (entry.tag == "n") is_hidden_single = test_hidden_single(cell, entry.value, mBoard->nonet(cell), out_tag);
                    else if (entry.tag == "c") is_hidden_single = test_hidden_single(cell, entry.value, mBoard->column(cell), out_tag);
                    else if (entry.tag == "r") is_hidden_single = test_hidden_single(cell, entry.value, mBoard->row(cell), out_tag);
                    else throw std::runtime_error("unknown tag");
                    if (!is_hidden_single) {
                        if (sVerbose) std::cout << "  [xHS] " << cell.coord() << " [NS]" << std::endl;
                    }
                    return !is_hidden_single;
                }), mBoard->mHiddenSingles.end());
}

template<class Set>
void Analyzer::find_hidden_singles(const Set &set) const {
    for (auto const &cell: set) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes!
        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell
            // is this a hidden single?
            std::string tag;
            if (!test_hidden_single(cell, value, mBoard->nonet(cell), tag)
             && !test_hidden_single(cell, value, mBoard->column(cell), tag)
             && !test_hidden_single(cell, value, mBoard->row(cell), tag)) continue;

            // yes! but do we already know about it?
            if (std::find_if(mBoard->mHiddenSingles.begin(), mBoard->mHiddenSingles.end(),
                       [cell](const auto &entry) { return cell.coord() == entry.coord; }) != mBoard->mHiddenSingles.end()) continue;

            // no! let's record it
            Board::HiddenSingle hs(cell.coord(), value, tag);
            mBoard->mHiddenSingles.push_back(hs);
            if (sVerbose) std::cout << "  [fHS] " << hs << std::endl;
            break;  // we're not going to find any other HS among the rest of the candidates for this cell
        }
    }
}

void Analyzer::find_hidden_singles() const {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    for (auto const &coord : mValueDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-single hidden singles in any of this cell's blocks
        find_hidden_singles(mBoard->nonet(cell));
        find_hidden_singles(mBoard->column(cell));
        find_hidden_singles(mBoard->row(cell));
    }
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-single hidden singles in any of this cell's blocks
        find_hidden_singles(mBoard->nonet(cell));
        find_hidden_singles(mBoard->column(cell));
        find_hidden_singles(mBoard->row(cell));
    }
}

namespace { // anon
template<class Set>
bool would_act(const Set &set, const Cell &c1, const Cell &c2, const Value &v1, const Value &v2) {
    bool would_act = false;
    for (auto const &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == c1 || other_cell == c2) continue;
        if (!other_cell.check(v1) && !other_cell.check(v2)) continue; // no impact on this cell

        would_act = true;
    }
    return would_act;
}
}

bool Analyzer::test_naked_pair(const Cell &c1, const Cell &c2) const {
    // are these two different cells?
    if (c1 == c2) return false;

    // yes! but are both cells in the same set?
    if (!(mBoard->nonet(c1) == mBoard->nonet(c2)
       || mBoard->column(c1) == mBoard->column(c2)
       || mBoard->row(c1) == mBoard->row(c2))) return false;

    // yes! but are both cells notes?
    if (!c1.isNote() || !c2.isNote()) return false;

    // yes! but do both cells have only a pair of candidates?
    if (c1.notes().count() != 2 || c2.notes().count() != 2) return false;

    // yes! but are they the same pairs of candidates?
    auto v11 = c1.notes().values().at(0);
    auto v12 = c1.notes().values().at(1);
    auto v21 = c2.notes().values().at(0);
    auto v22 = c2.notes().values().at(1);

    if (!((v11 == v21 && v12 == v22) || (v11 == v22 && v12 == v21))) return false;

    // yes! but would acting on them have an effet?
    if (mBoard->nonet(c1)  == mBoard->nonet(c2)  && !would_act(mBoard->nonet(c1), c1, c2, v11, v12)) return false;
    if (mBoard->column(c1) == mBoard->column(c2) && !would_act(mBoard->column(c1), c1, c2, v11, v12)) return false;
    if (mBoard->row(c1)    == mBoard->row(c2)    && !would_act(mBoard->row(c1), c1, c2, v11, v12)) return false;

    // yes!
    return true;
}

void Analyzer::filter_naked_pairs() const {
    mBoard->mNakedPairs.erase(std::remove_if(mBoard->mNakedPairs.begin(), mBoard->mNakedPairs.end(),
                [this](const auto &entry) {
                    bool is_naked_pair = test_naked_pair(mBoard->at(entry.coords.first), mBoard->at(entry.coords.second));
                    if (!is_naked_pair) {
                        if (sVerbose) std::cout << "  [xNP] " << entry << std::endl;
                    }
                    return !is_naked_pair;
                }), mBoard->mNakedPairs.end());
}

template<class Set>
void Analyzer::find_naked_pair(const Cell &cell, const Set &set) const {
    // have we already recorded this pair?
    if (std::find_if(mBoard->mNakedPairs.begin(), mBoard->mNakedPairs.end(),
                [cell](auto const &entry) { return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; })
            != mBoard->mNakedPairs.end()) return;

    // no! but...
    for (auto const &pair_cell : set) {
        // is this candidate pair cell good?
        if (!test_naked_pair(cell, pair_cell)) continue;

        // yes! let's record it
        Board::NakedPair np(std::make_pair(cell.coord(), pair_cell.coord()), std::make_pair(cell.notes().values().at(0), cell.notes().values().at(1)));
        mBoard->mNakedPairs.push_back(np);
        if (sVerbose) std::cout << "  [fNP] " << np << std::endl;
        break;
    }
}

void Analyzer::find_naked_pairs() const {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row,
    // or column, and no other candidates are possible in those cells, then those n candidates
    // are not possible elsewhere in that same block, row, or column.
    // Applied for n = 2
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // is this a note cell?
        if (!cell.isNote()) continue;
        // yes! but does it have only two notes?
        if (cell.notes().count() != 2) continue;

        // yes! let's see if we can find it a pair?
        find_naked_pair(cell, mBoard->nonet(cell));
        find_naked_pair(cell, mBoard->column(cell));
        find_naked_pair(cell, mBoard->row(cell));
    }
}

namespace { // anon
template<class Set>
bool would_act_on_set(std::vector<Coord> const &coords, Value const &value, std::string const &tag, Set const &set) {
    assert(set.tag() == tag);

    bool would_act = false;

    for (auto const &cell : set) {
        // is it a note cell?
        if (!cell.isNote()) continue;

        // yes! but is it a candidate?
        if (!cell.check(value)) continue;

        // yes! but is it one of the locked candidates?
        if (std::find(coords.begin(), coords.end(), cell.coord()) != coords.end()) continue;

        would_act = true;
        break;
    }

    return would_act;
}
}

void Analyzer::filter_locked_candidates() const {
    mBoard->mLockedCandidates.erase(std::remove_if(mBoard->mLockedCandidates.begin(), mBoard->mLockedCandidates.end(),
                [this](auto &entry) {
                    // are any of the coords in this entry still candidate notes for this value?
                    auto coords(entry.coords); // deep copy, for later printing of pristine entry
                    coords.erase(std::remove_if(coords.begin(), coords.end(),
                                [this, entry](auto const &coord) { auto const &cell = mBoard->at(coord); return cell.isValue() || !cell.check(entry.value); }),
                            coords.end());
                    if (coords.empty()) {
                        if (sVerbose) std::cout << "  [xLC] " << entry << std::endl;
                        return true;
                    }

                    // yes! but let's note if not all of them
                    if (coords.size() < entry.coords.size()) {
                        if(sVerbose) std::cout << "  [~LC] " << entry << " -> ";
                        entry.coords = coords;
                        if(sVerbose) std::cout << entry << std::endl;
                    }

                    // but would they still have an impact?
                    bool would_act = false;
                    switch (entry.tag.at(0)) {
                    case 'n':
                        would_act = would_act_on_set(entry.coords, entry.value, entry.tag, mBoard->nonet(mBoard->at(coords.at(0))));
                        break;
                    case 'c':
                        would_act = would_act_on_set(entry.coords, entry.value, entry.tag, mBoard->column(mBoard->at(coords.at(0))));
                        break;
                    case 'r':
                        would_act = would_act_on_set(entry.coords, entry.value, entry.tag, mBoard->row(mBoard->at(coords.at(0))));
                        break;
                    default:
                        throw std::runtime_error("unhandled case");
                    }
                    if (!would_act) {
                        if (sVerbose) std::cout << "  [xLC] " << entry << std::endl;
                        return true;
                    }

                    // yes! but are they stil by themselves within their recorded set?
                    // TODO
                    return false;
                }), mBoard->mLockedCandidates.end());
}

template<class Set1, class Set2>
void Analyzer::find_locked_candidate(const Cell &cell, const Value &value, Set1 &set_to_consider, Set2 &set_to_ignore) const {
    if (std::find_if(mBoard->mLockedCandidates.begin(), mBoard->mLockedCandidates.end(),
                [cell, value, set_to_ignore](const auto &entry) { return entry.contains(cell.coord(), value, set_to_ignore.tag()); }) != mBoard->mLockedCandidates.end()) {
        // we've already record the entry containing this cell, for this set and value
        return;
    }

    std::vector<Coord> lc_coords;
    lc_coords.push_back(cell.coord());

    for (auto const &other_cell : set_to_consider) {
        // is this a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is this the same cell?
        if (other_cell == cell) continue;

        // no! but is it a candidate for this value?
        if (!other_cell.check(value)) continue;

        // yes! but is it also a candidate?
        if (std::find(set_to_ignore.begin(), set_to_ignore.end(), other_cell) != set_to_ignore.end()) {

            // yes! record it
            lc_coords.push_back(other_cell.coord());

            // and continue the search
            continue;
        }

        // oh, this was disqualifying: we found another candidate cell is the set to ignore
        return;
    }

    // ensure that this set of locked candidates, if acted on, *would* have an effect
    for (auto const &other_cell : set_to_ignore) {
        // is it a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it a candidate?
        if (!other_cell.check(value)) continue;

        // yes! but is it one of the locked candidates?
        if (std::find(lc_coords.begin(), lc_coords.end(), other_cell.coord()) != lc_coords.end()) continue;

        // no! we found a note cell that is in the rest of the "set_to_ignore"
        // and also is a candidate for this value: we *would* act on it
        Board::LockedCandidates lc(lc_coords, value, set_to_ignore.tag());
        mBoard->mLockedCandidates.push_back(lc);
        if (sVerbose) std::cout << "  [fLC] " << lc << std::endl;
        return;
    }
}

template<class Set>
void Analyzer::find_locked_candidates(Set const &set) const {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks
    // Form 1:
    // When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same row/column,
    // then it is also not possible anywhere else in the same nonet
    // Form 2:
    // When a candidate is possible in a certain nonet and row/column, and it is not possible anywhere else in the same nonet,
    // then it is also not possible anywhere else in the same row/column

    for (auto const &cell: set) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but is it already recorded as a hidden single?
        if (std::find_if(mBoard->mHiddenSingles.begin(), mBoard->mHiddenSingles.end(),
                    [cell](auto const &entry) { return entry.coord == cell.coord(); })
                != mBoard->mHiddenSingles.end()) continue;

        // no! then for each candidate value in this note cell
        for (auto const &value : cell.notes().values()) {

            // form 1
            find_locked_candidate(cell, value, mBoard->row(cell), mBoard->nonet(cell));
            find_locked_candidate(cell, value, mBoard->column(cell), mBoard->nonet(cell));

            // form 2
            find_locked_candidate(cell, value, mBoard->nonet(cell), mBoard->row(cell));
            find_locked_candidate(cell, value, mBoard->nonet(cell), mBoard->column(cell));
        }
    }
}

void Analyzer::find_locked_candidates() const {
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-revealed hidden pairs in any of this cell's blocks
        find_locked_candidates(mBoard->nonet(cell));
        find_locked_candidates(mBoard->column(cell));
        find_locked_candidates(mBoard->row(cell));
    }
}

bool Analyzer::test_hidden_pair(const Cell &c1, const Cell &c2, const Value &v1, const Value &v2) const {
    // are they both note cells?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;

    // yes! but do they both have at least two entries?
    if (!(c1.notes().count() >= 2 && c2.notes().count() >= 2)) return false;

    // yes! but does at least one of them have more than two candidates?
    if (!(c1.notes().count() > 2 || c2.notes().count() > 2)) return false;

    // yes! but are both values still candidates for both notes?
    if (!(c1.check(v1) && c1.check(v2)))  return false;
    if (!(c2.check(v1) && c2.check(v2)))  return false;

    // both cells in the entry are still notes with one of them having strictly
    // more than two candidates and they both have both entry values as candidates
    return true;
}


void Analyzer::filter_hidden_pairs() const {
    mBoard->mHiddenPairs.erase(std::remove_if(mBoard->mHiddenPairs.begin(), mBoard->mHiddenPairs.end(),
                [this](auto const &entry) {
                    bool is_hidden_pair = test_hidden_pair(mBoard->at(entry.coords.first), mBoard->at(entry.coords.second), entry.values.first, entry.values.second);
                    if (!is_hidden_pair) {
                        if (sVerbose) std::cout << "  [xHP] " << entry << std::endl;
                    }
                    return !is_hidden_pair;
                }), mBoard->mHiddenPairs.end());
}

template<class Set>
void Analyzer::find_hidden_pair(const Cell &cell, const Value &v1, const Value &v2, const Set &set) const {
    assert(cell.isNote());
    assert(cell.notes().check(v1));
    assert(cell.notes().check(v2));

    // have we already recorded this cell into a pair?
    if (std::find_if(mBoard->mHiddenPairs.begin(), mBoard->mHiddenPairs.end(),
                [cell](auto const &entry) { return cell.coord() == entry.coords.first || cell.coord() == entry.coords.second; })
            != mBoard->mHiddenPairs.end()) return;

    // no! can we find another note cell with the same pair in the same set,
    // but no other cell with either candidate in the set?
    Cell *ppair_cell = NULL; // "the" other potential candidate
    bool condition_met = true;

    for (auto &other_cell : set) {
        // is this other cell a note cell?
        if (!other_cell.isNote()) continue;

        // yes! but is it a different cell?
        if (other_cell == cell) continue;

        // yes! but are the values possible candidates for it?
        if (!other_cell.check(v1) && !other_cell.check(v2)) continue;                      // no impact on algorithm; check next cell in row
        if (other_cell.check(v1) ^ other_cell.check(v2)) { condition_met = false; break; } // either v1 or v2 is disqualified
        assert(other_cell.check(v1) && other_cell.check(v2));

        // yes! but did we already find a pair candidate cell for this hidden pair?
        if (!ppair_cell) {              // no candidate yet
            ppair_cell = &other_cell;   // this is "the" other candidate
            continue;
        }
        else {                          // this is disqualifying: we have more than two candidates in the row
            condition_met = false;
            break;
        }
    }

    // did we, in fact, find another candidate?
    if (!ppair_cell) condition_met = false;

    // and also, is the candidate pair we found actionable (i.e. is it *not* a naked pair)?
    if (cell.notes().count() == 2
     && (ppair_cell && ppair_cell->notes().count() == 2)) condition_met = false;

    if (!condition_met) return; // we're done here

    // yes! let's record the entry
    Board::HiddenPair hp(std::make_pair(cell.coord(), ppair_cell->coord()), std::make_pair(v1, v2));
    mBoard->mHiddenPairs.push_back(hp);
    if (sVerbose) std::cout << "  [fHP] " << hp << std::endl;
}

template<class Set>
void Analyzer::find_hidden_pairs(Set const &set) const {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
    // When n candidates are possible in a certain set of n cells all in the same block, row, or column,
    // and those n candidates are not possible elsewhere in that same block, row, or column, then no other
    // candidates are possible in those cells.
    // Applied for n = 2

    for (auto const &cell: set) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have at least two candidate values?
        auto values = cell.notes().values();
        if (!(values.size() >= 2)) continue;

        // for each pair of candidates for this cell...
        for (auto pv1 = values.begin(); pv1 != values.end(); ++pv1) {
            for (auto pv2 = pv1 + 1; pv2 != values.end(); ++pv2) {
                assert(*pv2 != *pv1);

                // let's see if we can find a hidden pair in the three cell sets
                find_hidden_pair(cell, *pv1, *pv2, mBoard->nonet(cell));
                find_hidden_pair(cell, *pv1, *pv2, mBoard->column(cell));
                find_hidden_pair(cell, *pv1, *pv2, mBoard->row(cell));
            }
        }
    }
}

void Analyzer::find_hidden_pairs() const {
    for (auto const &coord : mValueDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-revealed hidden pairs in any of this cell's blocks
        find_hidden_pairs(mBoard->nonet(cell));
        find_hidden_pairs(mBoard->column(cell));
        find_hidden_pairs(mBoard->row(cell));
    }
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // are there now-revealed hidden pairs in any of this cell's blocks
        find_hidden_pairs(mBoard->nonet(cell));
        find_hidden_pairs(mBoard->column(cell));
        find_hidden_pairs(mBoard->row(cell));
    }
}

void Analyzer::analyze(Board &board) {
    assert(!mBoard || mBoard == &board);
    mBoard = &board;

    filter_notes();

    filter_naked_singles();
    filter_hidden_singles();
    filter_naked_pairs();
    filter_locked_candidates();
    filter_hidden_pairs();

    find_naked_singles();
    find_hidden_singles();
    find_naked_pairs();
    find_locked_candidates();
    find_hidden_pairs();
}
