// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-colorchain.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <cassert>
#include <memory>
#include <queue>
#include <string>
#include <unordered_set>

bool ColorChainFinding::cell_sees_both_colors(const Cell &cell, const Board &board) const {
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
        if (board.see_each_other(cell.coord(), colored_coord)) {
            if (color) { did_see_green = true; }
            else       { did_see_red = true; }
            if (did_see_green && did_see_red) break;
        }
    }
    return did_see_green && did_see_red;
}

void ColorChainFinding::print(std::ostream &outs) const {
    outs << "{";
    bool first = true;
    for (const auto &[coord, color] : cells) {
        if (!first) outs << ",";
        first = false;
        outs << coord << (color ? "🟩" : "🟥");
    }
    outs << "}#" << value;
}

bool ColorChainTechnique::test_color_chain(const Board &board, const ColorChainFinding &chain) {
    // A color chain is actionable if it can lead to eliminations via:
    // Rule 2: Two cells of the same color are in the same unit (conflict)
    // Rule 4: A cell can see cells of both colors

    // Check rule 2: cells of same color in the same unit
    auto [green_cells, red_cells] = chain.group_cells_by_color();
    if (board.any_see_each_other(green_cells)
     || board.any_see_each_other(red_cells)) {
        return true;
    }

    // Check rule 4: cells that can see both colors
    for (const auto &cell : board.cells()) {
        if (chain.cell_sees_both_colors(cell, board)) {
            return true;
        }
    }

    return false; // No actionable eliminations found
}

namespace {
    // Anchored on a coord (not a Cell): ported techniques are no longer friends
    // of Board, and the self-skip below only needs the anchor's coord anyway, so
    // there is no reason to round-trip Coord -> Cell -> Coord through the private
    // Board::at. Callers pass the unit via the public row/column/nonet(Coord)
    // overloads.
    template<class Set>
    bool find_strong_link_candidates(const Coord &coord, const Value &value, const Set &set, const Cell *&out_candidate) {
        bool did_find = false;

        const Cell *candidate;
        for (auto const &other_cell : set) {
            if (!other_cell.isNote()) continue;
            if (!other_cell.check(value)) continue;
            if (coord == other_cell.coord()) continue;

            if (did_find) { did_find = false; break; } // this is disqualifying: we found more than one candidate
            else { candidate = &other_cell; did_find = true; }
        }

        if (did_find) out_candidate = candidate;

        return did_find;
    }

// Build and record the first actionable color chain for `value` on `board`, if
// any. Was Analyzer::find_color_chains(const Value &); records into the
// technique's own bucket `out` (at most one entry -- find() short-circuits)
// instead of the member vector.
bool find_color_chains(const Board &board, const Value &value, FindingList &out) {
    bool did_find = false;

    std::unordered_set<Coord> visited_global;

    for (auto const &cell : board.cells()) {
        Coord coord = cell.coord();

        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have this value as candidate?
        if (!cell.check(value)) continue;

        // yes! but have we visited it before?
        if (visited_global.find(coord) != visited_global.end()) continue;

        // no! let's start building a new chain from this cell.
        ColorChainFinding chain(value);

        std::queue<std::pair<Coord, bool>> to_process;
        std::unordered_set<Coord> visited_local;

        to_process.push({coord, true});  // true = green
        chain.cells[coord] = true;
        visited_local.insert(coord);
        visited_global.insert(coord);

        while (!to_process.empty()) {
            auto [current_coord, current_color] = to_process.front();
            to_process.pop();

            // Find all cells strongly linked to this cell, addressing its units
            // by coord through Board's public overloads.
            std::vector<Cell> linked_cells;
            const Cell *other_cell = nullptr;
            if (find_strong_link_candidates(current_coord, value, board.row(current_coord), other_cell)) {
                linked_cells.push_back(*other_cell);
            }
            if (find_strong_link_candidates(current_coord, value, board.column(current_coord), other_cell)) {
                linked_cells.push_back(*other_cell);
            }
            if (find_strong_link_candidates(current_coord, value, board.nonet(current_coord), other_cell)) {
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
        if (!ColorChainTechnique::test_color_chain(board, chain)) continue;

        // yes! let's record it -- move the chain into the finding, so the dumped
        // and acted-on cell order is exactly the order the chain was built in.
        //
        // This *intentionally* changes the pre-#7 print order for SC. Before the
        // registry, act() consumed a chain the rebinding copy ctor had deep-copied
        // once more than the one the dump printed; libc++'s unordered_map copy ctor
        // prepends per bucket, so the extra copy reversed the order and the old dump
        // and Rule-2 blocks printed the same chain in opposite orders (see README).
        // #7 carries findings by a shallow shared_ptr<const> instead of deep-copying
        // them per state, so dump and act now read one shared object and necessarily
        // print one consistent order. Reproducing both old orders would require
        // re-copying the map inside apply() purely to mimic that STL quirk; we don't.
        // The elimination *set* is unchanged, and the black-box suite sorts before
        // comparing, so this is print-order-only.
        assert(out.empty());
        if (sVerbose) { std::cout << "  [fSC] "; chain.print(std::cout); std::cout << std::endl; }
        out.push_back(std::make_shared<ColorChainFinding>(std::move(chain)));
        did_find = true;
        break;
    }

    return did_find;
}
} // namespace

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
// Action is by applying two rules:
// Rule 2 - for a given color chain, if any row, column or nonet has the same color twice,
//          all candidates which share that color in the chain can be eliminated.
//
// Rule 4 - for a given color chain, if a candidate for the value that it *not* on the
//          chain can see two colors on the chain, then it can be eliminated.
bool ColorChainTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    for (Value val : value_range()) {
        if (::find_color_chains(board, val, out)) return true;
    }

    return false;
}

namespace {
bool act_on_color_chain_rule_2(Board &board, const std::vector<Coord> &coords, const Value &value, const std::string &color) {
    assert(!coords.empty());

    bool did_act = false;

    if (auto unit = board.any_see_each_other(coords)) {
       for (const Coord &coord : coords) {
           std::cout << "[SC] " << coord << " x" << value << " [" << tag(*unit) << color << "]" << std::endl;
           board.clear_note_at(coord, value);
       }
       did_act = true;
    }
    return did_act;
}
} // namespace

bool ColorChainTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    // Bucket invariant: every entry in this technique's bucket is a
    // ColorChainFinding (see NakedSingleTechnique::apply). The assert turns a
    // wrong-bucket wiring bug into a caught error, not UB.
    assert(dynamic_cast<const ColorChainFinding *>(mine.front().get()));
    auto const *chain = static_cast<const ColorChainFinding *>(mine.front().get());

    bool did_act = false;

    // Check rule 2: cells of same color in the same unit
    auto [green_cells, red_cells] = chain->group_cells_by_color();
    bool eliminated_a = act_on_color_chain_rule_2(board, green_cells, chain->value, "🟩");
    bool eliminated_b = act_on_color_chain_rule_2(board, red_cells, chain->value, "🟥");

    if (eliminated_a || eliminated_b) {
        did_act = true;
    }

    // Check rule 4: cells that can see both colors
    for (const auto &cell : board.cells()) {
        if (chain->cell_sees_both_colors(cell, board)) {
            std::cout << "[SC] " << cell.coord() << " x" << chain->value << " [👀🟩🟥]" << std::endl;
            board.clear_note_at(cell.coord(), chain->value);
            did_act = true;
        }
    }

    mine.clear();

    assert(did_act);
    return did_act;
}
