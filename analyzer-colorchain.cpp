// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>
#include <queue>

bool Analyzer::ColorChain::cell_sees_both_colors(const Cell &cell, const Board *board) const {
    // is this cell a note?
    if (!cell.isNote()) return false;

    // yes! but is it a candidate for this chain's value?
    if (!cell.check(value)) return false;

    // yes! but is it *on* the chain?
    if (cells.find(cell.coord()) != cells.end()) return false;

    // no! let's look at every note in the chain and see if this cell sees both colors
    bool did_see_green = false;
    bool did_see_red = false;

    for (const auto &[colored_coord, color] : cells) {
        std::string tag;
        if (board->see_each_other(cell.coord(), colored_coord, tag)) {
            if (color) { did_see_green = true; }
            else       { did_see_red = true; }
            if (did_see_green && did_see_red) break;
        }
    }
    return did_see_green && did_see_red;
}

bool Analyzer::test_color_chain(const ColorChain &chain) const {
    // A color chain is actionable if it can lead to eliminations via:
    // Rule 2: Two cells of the same color are in the same unit (conflict)
    // Rule 4: A cell can see cells of both colors

    // Check rule 2: cells of same color in the same unit
    auto [green_cells, red_cells] = chain.group_cells_by_color();
    std::string tag;
    if (mBoard->any_see_each_other(green_cells, tag)
     || mBoard->any_see_each_other(red_cells, tag)) {
        return true;
    }

    // Check rule 4: cells that can see both colors
    for (const auto &cell : mBoard->cells()) {
        if (chain.cell_sees_both_colors(cell, mBoard)) {
            return true;
        }
    }

    return false; // No actionable eliminations found
}

namespace {
    template<class Set>
    bool find_strong_link_candidates(const Cell &cell, const Value &value, const Set &set, const Cell *&out_candidate) {
        bool did_find = false;

        const Cell *candidate;
        for (auto const &other_cell : set) {
            if (!other_cell.isNote()) continue;
            if (!other_cell.check(value)) continue;
            if (cell == other_cell) continue;

            if (did_find) { did_find = false; break; } // this is disqualifying: we found more than one candidate
            else { candidate = &other_cell; did_find = true; }
        }

        if (did_find) out_candidate = candidate;

        return did_find;
    }

}

bool Analyzer::find_color_chains(const Value &value) {
    bool did_find = false;

    std::unordered_set<Coord> visited_global;

    for (auto const &cell : mBoard->cells()) {
        Coord coord = cell.coord();

        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have this value as candidate?
        if (!cell.check(value)) continue;

        // yes! but have we visited it before?
        if (visited_global.find(coord) != visited_global.end()) continue;

        // no! let's start building a new chain from this cell.
        ColorChain chain;
        chain.value = value;

        std::queue<std::pair<Coord, bool>> to_process;
        std::unordered_set<Coord> visited_local;

        to_process.push({coord, true});  // true = green
        chain.cells[coord] = true;
        visited_local.insert(coord);
        visited_global.insert(coord);

        while (!to_process.empty()) {
            auto [current_coord, current_color] = to_process.front();
            to_process.pop();

            const Cell &current_cell = mBoard->at(current_coord);

            // Find all cells strongly linked to this cell
            std::vector<Cell> linked_cells;
            const Cell *other_cell = nullptr;
            if (find_strong_link_candidates(current_cell, value, mBoard->row(current_cell), other_cell)) {
                linked_cells.push_back(*other_cell);
            }
            if (find_strong_link_candidates(current_cell, value, mBoard->column(current_cell), other_cell)) {
                linked_cells.push_back(*other_cell);
            }
            if (find_strong_link_candidates(current_cell, value, mBoard->nonet(current_cell), other_cell)) {
                linked_cells.push_back(*other_cell);
            }

            // Add linked cells to the chain with appropriate colors
            for (const Cell &linked_cell : linked_cells) {
                Coord linked_coord = linked_cell.coord();

                if (visited_local.find(linked_coord) != visited_local.end()) continue;

                // New cell - add with opposite color and queue for processing
                bool opposite_color = !current_color;
                chain.cells[linked_coord] = opposite_color;
                to_process.push({linked_coord, opposite_color});
                visited_local.insert(linked_coord);
                visited_global.insert(linked_coord);
            }
        }

        // ok, we have a chain, but is it large enough?
        if (chain.cells.size() < 2) continue;

        // yes! but is it actionable?
        if (!test_color_chain(chain)) continue;

        // yes! let's record it
        assert(mColorChains.empty());
        mColorChains.push_back(chain);
        if (sVerbose) std::cout << "  [fSC] " << chain << std::endl;
        did_find = true;
        break;
    }

    return did_find;
}

bool Analyzer::find_color_chains() {
    // https://www.sudokuwiki.org/Simple_Colouring
    //
    // Simple Coloring, also known as Single's Chains, is a chaining strategy.
    //
    // For a given candidate value, we are building a graph of candidate cells for this value,
    // linked by 'bi-location' links, and sporting alternate 'green' and 'red' colors.
    //
    // A 'bi-location' link is a link between a candidate for a given value and another
    // candidate for the same value in the same row, column or nonet, *if* there are no
    // additional candidate for the same value in the same row, column or nonet.
    //
    // The resulting graph is a "color chain".
    //
    // Action is by applying tow rules:
    // Rule 2 - for a given color chain, if any row, column or nonet has the same color twice,
    //          all candidates which share that color in the chain can be eliminated.
    //
    // Rule 4 - for a given color chain, if a candidate for the value that it *not* on the
    //          chain can see two colors on the chain, then it can be eliminated.
    bool did_find = false;

    for (Value val : value_range(kOne, kUnset)) {
        did_find = find_color_chains(val);
        if (!did_find) continue;
        break;
    }

    return did_find;
}

namespace {
bool act_on_color_chain_rule_2(Board *board, const std::vector<Coord> &coords, const Value &value, const std::string &color) {
    assert(!coords.empty());

    bool did_act = false;

    std::string tag;
    if (board->any_see_each_other(coords, tag)) {
       for (const Coord &coord : coords) {
           std::cout << "[SC] " << coord << " x" << value << " [" << tag << color << "]" << std::endl;
           board->clear_note_at(coord, value);
       }
       did_act = true;
    }
    return did_act;
}
}

bool Analyzer::act_on_color_chain() {
    bool did_act = false;

    if (mColorChains.empty()) return did_act;
    assert(mColorChains.size() == 1);

    const auto &chain = mColorChains.back();

    // Check rule 2: cells of same color in the same unit
    auto [green_cells, red_cells] = chain.group_cells_by_color();
    bool eliminated_a = act_on_color_chain_rule_2(mBoard, green_cells, chain.value, "🟩");
    bool eliminated_b = act_on_color_chain_rule_2(mBoard, red_cells, chain.value, "🟥");

    if (eliminated_a || eliminated_b) {
        did_act = true;
    }

    // Check rule 4: cells that can see both colors
    for (const auto &cell : mBoard->cells()) {
        if (chain.cell_sees_both_colors(cell, mBoard)) {
            std::cout << "[SC] " << cell.coord() << " x" << chain.value << " [👀🟩🟥]" << std::endl;
            mBoard->clear_note_at(cell.coord(), chain.value);
            did_act = true;
        }
    }

    mColorChains.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::ColorChain &chain) {
    outs << "{";
    bool first = true;
    for (const auto &[coord, color] : chain) {
        if (!first) outs << ",";
        first = false;
        outs << coord << (color ? "🟩" : "🟥");
    }
    outs << "}#" << chain.value;
    return outs;
}
