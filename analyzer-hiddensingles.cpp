// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-hiddensingles.h"
#include "board.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <cassert>
#include <memory>
#include <optional>

namespace {
// File-local: Hidden Singles has no whitebox hooks, so nothing outside this TU
// needs to name or downcast the finding. print() emits the same bytes the old
// free operator<<(ostream, Analyzer::HiddenSingle) did: "coord#value[unit]".
struct HiddenSingleFinding : Finding {
    Coord coord;
    Value value;
    Unit  unit;
    HiddenSingleFinding(const Coord &c, const Value &v, Unit u) : coord(c), value(v), unit(u) { }
    void print(std::ostream &o) const override { o << coord << "#" << value << "[" << tag(unit) << "]"; }
};

// Is `value` a hidden single for `cell` within `set`? Returns the unit kind when
// cell is the *only* candidate for value in set, nullopt otherwise. Was the
// Analyzer::test_hidden_single member template; a pure query on the passed-in
// cell/set, so it drops to a file-local free function with the technique port.
template<class Set>
std::optional<Unit> test_hidden_single(const Cell &cell, const Value &value, const Set &set) {
    if (!cell.isNote()) return std::nullopt;
    if (cell.notes().count() <= 1) return std::nullopt; // either a naked single, or an impossibility
    if (!cell.check(value)) return std::nullopt;        // not a candidate for value any longer

    for (auto const &other_cell : set) {
        if (other_cell == cell) continue;               // do not consider the current cell
        assert(other_cell.isNote() || other_cell.value() != value);
        if (other_cell.isValue()) continue;             // only considering note cells
        if (!other_cell.notes().check(value)) continue; // this note cell is _not_ a candidate

        return std::nullopt;
    }
    return set.kind();
}
} // namespace

// A hidden single arises when there is only one possible cell for a candidate.
// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
bool HiddenSingleTechnique::find(const Board &board, FindingList &out) const {
    bool did_find = false;

    for (auto const &cell : board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes!
        for (auto const &value : cell.notes().values()) { // for each candidate value in this note cell
            // is this a hidden single in its row, column, or nonet?
            auto unit = test_hidden_single(cell, value, board.row(cell));
            if (!unit) unit = test_hidden_single(cell, value, board.column(cell));
            if (!unit) unit = test_hidden_single(cell, value, board.nonet(cell));
            if (!unit) continue;

            // yes! let's record it. (No duplicate-coord guard: the bucket is
            // cleared each analyze(), the outer loop visits each cell once, and we
            // break after the first hidden single found for a cell -- so no coord
            // can recur. The old scan of the member vector for a duplicate coord
            // was dead for exactly these reasons.)
            auto finding = std::make_shared<HiddenSingleFinding>(cell.coord(), value, *unit);
            if (sVerbose) { std::cout << "  [fHS] "; finding->print(std::cout); std::cout << std::endl; }
            out.push_back(std::move(finding));
            did_find = true;
            break;  // we're not going to find any other HS among the rest of the candidates for this cell
        }
    }
    return did_find;
}

bool HiddenSingleTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    // singles can be acted on all at once
    for (auto const &f : mine) {
        // Bucket invariant: see NakedSingleTechnique::apply for the rationale.
        // The assert turns a wrong-bucket wiring bug into a caught error, not UB.
        assert(dynamic_cast<const HiddenSingleFinding *>(f.get()));
        auto const *hs = static_cast<const HiddenSingleFinding *>(f.get());
        // ::tag -- the free tag(Unit) from board.h; the inherited Technique::tag()
        // member (0 args) would otherwise shadow it inside this member function.
        std::cout << "[HS] " << hs->coord << " =" << hs->value << " [" << ::tag(hs->unit) << "]" << std::endl;
        board.set_value_at(hs->coord, hs->value);
    }
    mine.clear();
    return true;
}
