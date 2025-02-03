// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"

bool Analyzer::test_naked_single(const Cell &cell) const {
    return cell.isNote()
        && cell.notes().count() == 1;
}

void Analyzer::filter_naked_singles() {
    mNakedSingles.erase(std::remove_if(mNakedSingles.begin(), mNakedSingles.end(),
                [this](const auto &coord) {
                    auto &cell = mBoard->at(coord);
                    bool is_naked_single = test_naked_single(cell);
                    if (!is_naked_single) {
                        if (sVerbose) std::cout << "  [xNS] " << coord << std::endl;
                    }
                    return !is_naked_single;
                }), mNakedSingles.end());
}

void Analyzer::find_naked_singles() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A naked single arises when there is only one possible candidate for a cell
    for (auto const &coord : mNotesDirtySet) {
        auto &cell = mBoard->at(coord);

        // is this a naked single?
        if (!test_naked_single(cell)) continue;

        // yes! but do we already know about it?
        if (std::find(mNakedSingles.begin(), mNakedSingles.end(), cell.coord()) != mNakedSingles.end()) continue;

        // no! let's record it
        mNakedSingles.push_back(cell.coord());
        if (sVerbose) std::cout << "  [fNS] " << cell.coord() << std::endl;
    }
}

bool Analyzer::act_on_naked_single() {
    if (mNakedSingles.empty()) { return false; }

    auto const coord = mNakedSingles.back();
    mNakedSingles.pop_back();

    auto &cell = mBoard->at(coord);

    std::vector<Value> values = cell.notes().values();
    assert(values.size() == 1);
    Value value = values.at(0);

    std::cout << "[NS] " << cell.coord() << " =" << value << std::endl;
    mBoard->set_value_at(coord, value);

    return true;
}
