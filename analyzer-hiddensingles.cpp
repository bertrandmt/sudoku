// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

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

bool Analyzer::find_hidden_singles() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    bool did_find = false;

    for (auto const &cell: mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes!
        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell
            // is this a hidden single?
            std::string tag;
            if (!test_hidden_single(cell, value, mBoard.row(cell), tag)
             && !test_hidden_single(cell, value, mBoard.column(cell), tag)
             && !test_hidden_single(cell, value, mBoard.nonet(cell), tag)) continue;

            // yes! but do we already know about it?
            if (std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(),
                       [cell](const auto &entry) { return cell.coord() == entry.coord; }) != mHiddenSingles.end()) continue;

            // no! let's record it
            HiddenSingle hs(cell.coord(), value, tag);
            mHiddenSingles.push_back(hs);
            if (sVerbose) std::cout << "  [fHS] " << hs << std::endl;
            did_find = true;
            break;  // we're not going to find any other HS among the rest of the candidates for this cell
        }
    }
    return did_find;
}

bool Analyzer::act_on_hidden_single() {
    bool did_act = false;

    if (mHiddenSingles.empty()) return did_act;

    for (auto const &entry : mHiddenSingles) {
        std::cout << "[HS] " << entry.coord << " =" << entry.value << " [" << entry.tag << "]" << std::endl;
        mBoard.set_value_at(entry.coord, entry.value);
        did_act = true;
    }
    mHiddenSingles.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::HiddenSingle &hs) {
    return outs << hs.coord << "#" << hs.value << "[" << hs.tag << "]";
}
