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

void Analyzer::filter_hidden_singles() {
    mHiddenSingles.erase(std::remove_if(mHiddenSingles.begin(), mHiddenSingles.end(),
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
                }), mHiddenSingles.end());
}

template<class Set>
void Analyzer::find_hidden_singles(const Set &set) {
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
            if (std::find_if(mHiddenSingles.begin(), mHiddenSingles.end(),
                       [cell](const auto &entry) { return cell.coord() == entry.coord; }) != mHiddenSingles.end()) continue;

            // no! let's record it
            HiddenSingle hs(cell.coord(), value, tag);
            mHiddenSingles.push_back(hs);
            if (sVerbose) std::cout << "  [fHS] " << hs << std::endl;
            break;  // we're not going to find any other HS among the rest of the candidates for this cell
        }
    }
}

void Analyzer::find_hidden_singles() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A hidden single arises when there is only one possible cell for a candidate
    for (auto const &coord : mValueDirtySet) {
        // are there now-single hidden singles in any of this cell's blocks
        find_hidden_singles(mBoard->nonet(coord));
        find_hidden_singles(mBoard->column(coord));
        find_hidden_singles(mBoard->row(coord));
    }
    for (auto const &coord : mNotesDirtySet) {
        // are there now-single hidden singles in any of this cell's blocks
        find_hidden_singles(mBoard->nonet(coord));
        find_hidden_singles(mBoard->column(coord));
        find_hidden_singles(mBoard->row(coord));
    }
}

bool Analyzer::act_on_hidden_single() {
    if (mHiddenSingles.empty()) { return false; }

    auto const entry = mHiddenSingles.back();
    mHiddenSingles.pop_back();

    std::cout << "[HS] " << entry.coord << " =" << entry.value << " [" << entry.tag << "]" << std::endl;
    mBoard->set_value_at(entry.coord, entry.value);

    return true;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::HiddenSingle &hs) {
    return outs << hs.coord << "#" << hs.value << "[" << hs.tag << "]";
}
