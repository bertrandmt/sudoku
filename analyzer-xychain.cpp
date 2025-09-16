// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>
#include <unordered_set>
#include <algorithm>

size_t Analyzer::test_xychain(const Value &value, const std::vector<Coord> &chain) const {
    // validate that the chain chains properly, and that the end candidate value is also a
    // candidate for the initial cell.
    Value other_value = value;
    for (const auto &coord : chain) {
        Value this_value = other_value;
        const Cell &cell = mBoard.at(coord);

        assert(cell.isNote());
        assert(cell.notes().count() == 2);
        if (!cell.check(this_value)) return 0;
        other_value = cell.other_value(this_value);
    }
    // is the last "other_value" is the incoming candidate value
    if (other_value != value) return 0;

    // yes! count eliminations
    size_t num_elim = 0;
    for (const auto &cell : mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but is value a candidate for it?
        if (!cell.check(value)) continue;

        // yes! but is it on the chain?
        if (std::find(chain.begin(), chain.end(), cell.coord()) != chain.end()) continue;

        // no! but can it see both ends?
        std::string tag;
        if (!mBoard.see_each_other(cell.coord(), chain.front(), tag)) continue;
        if (!mBoard.see_each_other(cell.coord(), chain.back(), tag)) continue;

        // yes! count it
        num_elim++;
    }

    return num_elim;
}

namespace {
    template<class Set>
    void select_chain_candidates(const Cell &current, Value value, const Set &set, const std::unordered_set<Coord> &visited, std::unordered_set<Cell> &chain_candidates) {
        assert(set.contains(current));

        for (auto const &cell : set) {
            // is this not the current cell
            if (cell == current) continue;

            // yes! but has it been visited before?
            if (visited.count(cell.coord())) continue;

            // yes! but is it a note?
            if (!cell.isNote()) continue;

            // yes! but does it have only two candidates?
            if (cell.notes().count() != 2) continue;

            // yes! but does it have value as candidate?
            if (!cell.check(value)) continue;

            // yes! ok, this is a bona fide chain candidate; record it
            // unordered_set automatically handles duplicates
            chain_candidates.insert(cell);
        }
    }
}

bool Analyzer::find_xychain(const Cell &cell, const Value &value) {
    assert(cell.isNote());
    assert(cell.notes().count() == 2);
    assert(cell.check(value));

    std::vector<Coord> chain;
    std::unordered_set<Coord> visited;

    // Try to extend chain recursively
    std::function<bool(const Cell&, Value)> extend_chain = [&](const Cell &cell, Value incoming_link_value) -> bool {
        assert(cell.isNote());
        assert(cell.check(incoming_link_value));
        assert(cell.notes().count() == 2);

        bool did_find = false;

        // select cells that can see current cell and share its "other" value
        Value common_link_value = cell.other_value(incoming_link_value);
        std::unordered_set<Cell> candidates;
        select_chain_candidates(cell, common_link_value, mBoard.row(cell), visited, candidates);
        select_chain_candidates(cell, common_link_value, mBoard.column(cell), visited, candidates);
        select_chain_candidates(cell, common_link_value, mBoard.nonet(cell), visited, candidates);

        for (const auto &next_cell : candidates) {
            // proactively extend the chain with next_cell
            chain.push_back(next_cell.coord());
            visited.insert(next_cell.coord());

            // is the chain valid and would acting on it have an impact
            Value next_link_value = next_cell.other_value(common_link_value);
            size_t num_elim = test_xychain(next_link_value, chain);
            if (num_elim > 0) {
                XYChain xyc{next_link_value, chain, num_elim};

                // yes! but is it already recorded?
                if (std::find(mXYChains.begin(), mXYChains.end(), xyc) == mXYChains.end()) {

                    // no! let's record it
                    mXYChains.insert(xyc);

                    if (xyc == *mXYChains.begin()) {
                        if (sVerbose) std::cout << "  [fXY] " << xyc << std::endl;
                        did_find = true;
                    }

                    // and also only keep the most desirable chain
                    if (mXYChains.size() > 1) mXYChains.erase(std::prev(mXYChains.end()));
                }
            }

            // recurse
            did_find |= extend_chain(next_cell, common_link_value);

            // backtrack
            chain.pop_back();
            visited.erase(next_cell.coord());
        }

        return did_find;
    };

    bool did_find = false;

    chain.push_back(cell.coord());
    visited.insert(cell.coord());

    did_find |= extend_chain(cell, value);

    return did_find;
}

bool Analyzer::find_xychains() {
    // https://www.sudokuwiki.org/XY_Chains
    // An XY-Chain is a sequence of XY-cells (cells with exactly 2 candidates)
    // where each adjacent pair shares exactly one candidate.
    // If the chain starts and ends with the same candidate, that candidate
    // can be eliminated from cells that can see both chain ends.
    //
    // For this heuristic, we will find all chains, sort them by number of
    // eliminations (greater is better) and length (shorter is better), and act
    // only on the most desirable chain.
    assert(mXYChains.empty());
    bool did_find = false;

    for (const auto &cell : mBoard.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have only two candidates?
        if (cell.notes().count() != 2) continue;

        // yes! attempt to build chains from this cell for each candidate value
        did_find |= find_xychain(cell, cell.notes().values()[0]);
        did_find |= find_xychain(cell, cell.notes().values()[1]);
    }

    return did_find;
}

template<class Set>
bool Analyzer::act_on_xychain(const XYChain &entry, const Set &chain_front_set) {
    bool did_act = false;

    // For each cell that can see the other end of the chain, eliminate the chain value
    for (const auto &cell : chain_front_set) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but is it a candidate for the entry's value?
        if (!cell.check(entry.value)) continue;

        // yes! but is it on the chain?
        if (std::find(entry.chain.begin(), entry.chain.end(), cell.coord()) != entry.chain.end()) continue;

        // no! but can it see the other end of the chain?
        std::string tag;
        if (!mBoard.see_each_other(cell.coord(), entry.chain.back(), tag)) continue;

        // yes! ELIMINATE!
        std::cout << "[XY] " << cell.coord() << " x" << entry.value
                  << " ({" << entry.chain.front() << ":..:" << entry.chain.back() << "}#" << entry.value << ")" << std::endl;
        mBoard.clear_note_at(cell.coord(), entry.value);
        did_act = true;
    }

    return did_act;
}

bool Analyzer::act_on_xychain() {
    bool did_act = false;

    if (mXYChains.empty()) return did_act;

    const auto &entry = *mXYChains.begin();

    did_act |= act_on_xychain(entry, mBoard.row(entry.chain.front()));
    did_act |= act_on_xychain(entry, mBoard.column(entry.chain.front()));
    did_act |= act_on_xychain(entry, mBoard.nonet(entry.chain.front()));

    mXYChains.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::XYChain &xyc) {
    outs << "{";
    for (size_t i = 0; i < xyc.chain.size(); i++) {
        if (i > 0) outs << ":";
        outs << xyc.chain[i];
    }
    outs << "}#" << xyc.value
         << "x" << xyc.num_elim;
    return outs;
}
