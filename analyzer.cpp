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

        auto &cell = mBoard->at(coord);         // review the notes in
        filter_notes(mBoard->nonet(cell));      // each cell in this cell's nonet, and
        filter_notes(mBoard->column(cell));     //                      ... column, and
        filter_notes(mBoard->row(cell));        //                      ... row
    }
}

void Analyzer::analyze() {
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

bool Analyzer::act(const bool singles_only) {
    bool did_act = false;

    did_act = act_on_naked_single();
    if (!did_act) did_act = act_on_hidden_single();

    if (!singles_only) {
        if (!did_act) did_act = act_on_naked_pair();
        if (!did_act) did_act = act_on_locked_candidate();
        if (!did_act) did_act = act_on_hidden_pair();
    }

    return did_act;
}

std::ostream &operator<<(std::ostream &outs, Analyzer const &a) {
    // naked singles
    outs << "[NS](" << a.mNakedSingles.size() << ") {";
    bool is_first = true;
    for (auto const &coord : a.mNakedSingles) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << coord;
    }
    outs << "}" << std::endl
   // hidden singles
         << "[HS](" << a.mHiddenSingles.size() << ") {";
    is_first = true;
    for (auto const &entry: a.mHiddenSingles) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << entry;
    }
    outs << "}" << std::endl
    // naked pairs
         << "[NP](" << a.mNakedPairs.size() << ") {";
    is_first = true;
    for (auto const &entry: a.mNakedPairs) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}" << std::endl
     // locked candidates
         << "[LC](" << a.mLockedCandidates.size() << ") {";
    is_first = true;
    for (auto const &entry: a.mLockedCandidates) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}" << std::endl
    // hidden pairs
         << "[HP](" << a.mHiddenPairs.size() << ") {";
    is_first = true;
    for (auto const &entry: a.mHiddenPairs) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}";

    return outs;
}
