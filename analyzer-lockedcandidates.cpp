// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

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

void Analyzer::filter_locked_candidates() {
    mLockedCandidates.erase(std::remove_if(mLockedCandidates.begin(), mLockedCandidates.end(),
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
                        would_act = would_act_on_set(entry.coords, entry.value, entry.tag, mBoard->nonet(coords.at(0)));
                        break;
                    case 'c':
                        would_act = would_act_on_set(entry.coords, entry.value, entry.tag, mBoard->column(coords.at(0)));
                        break;
                    case 'r':
                        would_act = would_act_on_set(entry.coords, entry.value, entry.tag, mBoard->row(coords.at(0)));
                        break;
                    default:
                        throw std::runtime_error("unhandled case");
                    }
                    if (!would_act) {
                        if (sVerbose) std::cout << "  [xLC] " << entry << std::endl;
                        return true;
                    }

                    // yes! we're keeping this entry
                    return false;
                }), mLockedCandidates.end());
}

template<class Set1, class Set2>
void Analyzer::find_locked_candidate(const Cell &cell, const Value &value, Set1 &set_to_consider, Set2 &set_to_ignore) {
    if (std::find_if(mLockedCandidates.begin(), mLockedCandidates.end(),
                [cell, value, set_to_ignore](const auto &entry) { return entry.contains(cell.coord(), value, set_to_ignore.tag()); }) != mLockedCandidates.end()) {
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
        LockedCandidates lc(lc_coords, value, set_to_ignore.tag());
        mLockedCandidates.push_back(lc);
        if (sVerbose) std::cout << "  [fLC] " << lc << std::endl;
        return;
    }
}

template<class Set>
void Analyzer::find_locked_candidates(Set const &set) {
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
        if (std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(),
                    [cell](auto const &entry) { return entry.coord == cell.coord(); })
                != mHiddenSingles.end()) continue;

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

void Analyzer::find_locked_candidates() {
    for (auto const &coord : mValueDirtySet) {
        // are there now-revealed hidden pairs in any of this cell's blocks
        find_locked_candidates(mBoard->nonet(coord));
        find_locked_candidates(mBoard->column(coord));
        find_locked_candidates(mBoard->row(coord));
    }
    for (auto const &coord : mNotesDirtySet) {
        // are there now-revealed hidden pairs in any of this cell's blocks
        find_locked_candidates(mBoard->nonet(coord));
        find_locked_candidates(mBoard->column(coord));
        find_locked_candidates(mBoard->row(coord));
    }
}

template<class Set>
bool Analyzer::act_on_locked_candidate(const LockedCandidates &entry, Set &set) {
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
        mBoard->clear_note_at(other_cell.coord(), entry.value);
        acted_on_locked_candidates = true;
    }

    //assert(acted_on_locked_candidates);
    return acted_on_locked_candidates;
}

bool Analyzer::act_on_locked_candidate() {
    if (mLockedCandidates.empty()) { return false; }

    auto const entry = mLockedCandidates.back(); // copy
    mLockedCandidates.pop_back();

    switch (entry.tag[0]) {
    case 'n':
        (void) act_on_locked_candidate(entry, mBoard->nonet(entry.coords.at(0)));
        break;
    case 'c':
        (void) act_on_locked_candidate(entry, mBoard->column(entry.coords.at(0)));
        break;
    case 'r':
        (void) act_on_locked_candidate(entry, mBoard->row(entry.coords.at(0)));
        break;
    }

    return true;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::LockedCandidates &lc) {
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
