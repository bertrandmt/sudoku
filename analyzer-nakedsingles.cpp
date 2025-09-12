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
    mNakedSingles.clear();
}

bool Analyzer::find_naked_singles() {
    // https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
    // A naked single arises when there is only one possible candidate for a cell
    bool did_find = false;

    for (auto const &cell: mBoard->cells()) {
        // is this a naked single?
        if (!test_naked_single(cell)) continue;

        assert(std::find_if(mNakedSingles.begin(), mNakedSingles.end(),
                   [cell](const auto &entry) { return cell.coord() == entry.coord; }) == mNakedSingles.end());

        // yes! let's record it
        NakedSingle ns(cell.coord(), cell.notes().values().at(0));
        mNakedSingles.push_back(ns);
        if (sVerbose) std::cout << "  [fNS] " << ns << std::endl;
        did_find = true;
    }

    return did_find;
}

bool Analyzer::act_on_naked_single() {
    bool did_act = false;

    if (mNakedSingles.empty()) return did_act;

    // singles can be acted on all at once
    for (auto const &entry: mNakedSingles) {
        auto &cell = mBoard->at(entry.coord);

        std::vector<Value> values = cell.notes().values();
        assert(values.size() == 1);
        Value value = values.at(0);

        std::cout << "[NS] " << cell.coord() << " =" << value << std::endl;
        mBoard->set_value_at(cell.coord(), value);
        did_act = true;
    }

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::NakedSingle &ns) {
    return outs << ns.coord << "#" << ns.value;
}
