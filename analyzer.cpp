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

template<class Set>
void Analyzer::filter_notes(Cell &cell, const Set &set) {
    // note cell; update its own notes from all the value cells in the same set
    if (cell.isNote()) {
        for (auto const &other_cell : set) {
            // is the other cell a value note?
            if (other_cell.isNote()) continue;
            // yes, but is the value of other_cell already checked off cell's notes
            if (!cell.check(other_cell.value())) continue;

            if (sVerbose) std::cout << "  [FNn] " << cell.coord() << " x" << other_cell.value()
                      << " " << set.tag() << "(" << other_cell.coord() << ")" << std::endl;
            mBoard->clear_note_at(cell.coord(), other_cell.value());
        }
    }
    // value cell; let's update notes in note cells in the same set
    else {
        assert(cell.isValue());
        for (auto &other_cell : set) {
            // is other_cell a note cell?
            if (other_cell.isValue()) continue;
            // yes, but is the value of cell already checked off other_cell's notes
            if (!other_cell.check(cell.value())) continue;

            if (sVerbose) std::cout << "  [FNv] " << other_cell.coord() << " x" << cell.value()
                      << " " << set.tag() << "(" << cell.coord() << ")" << std::endl;
            mBoard->clear_note_at(other_cell.coord(), cell.value());
        }
    }
}

void Analyzer::filter_notes() {
    for (auto &cell: mBoard->cells()) {
        filter_notes(cell, mBoard->nonet(cell));
        filter_notes(cell, mBoard->column(cell));
        filter_notes(cell, mBoard->row(cell));
    }
}

void Analyzer::analyze() {
    filter_notes();

    mNakedSingles.clear();
    mHiddenSingles.clear();
    mNakedPairs.clear();
    mLockedCandidates.clear();
    mHiddenPairs.clear();
    mXWings.clear();
    mColorChains.clear();

    bool did_find = false;
    did_find = find_naked_singles();
    if (!did_find) did_find = find_hidden_singles();
    if (!did_find) did_find = find_naked_pairs();
    if (!did_find) did_find = find_locked_candidates();
    if (!did_find) did_find = find_hidden_pairs();
    if (!did_find) did_find = find_xwings();
    if (!did_find) did_find = find_color_chains();
}

bool Analyzer::act(const bool singles_only) {
    bool did_act = false;

    did_act = act_on_naked_single();
    if (!did_act) did_act = act_on_hidden_single();

    if (!singles_only) {
        if (!did_act) did_act = act_on_naked_pair();
        if (!did_act) did_act = act_on_locked_candidate();
        if (!did_act) did_act = act_on_hidden_pair();
        if (!did_act) did_act = act_on_xwing();
        if (!did_act) did_act = act_on_color_chain();
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
    outs << "}" << std::endl
    // x-wings
         << "[XW](" << a.mXWings.size() << ") {";
    is_first = true;
    for (auto const &entry: a.mXWings) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}" << std::endl
    // color chains (a.k.a. single's chains)
         << "[SC](" << a.mColorChains.size() << ") {";
    is_first = true;
    for (auto const &entry: a.mColorChains) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        outs << "{" << entry << "}";
    }
    outs << "}";

    return outs;
}
