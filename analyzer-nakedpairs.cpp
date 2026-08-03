// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-nakedpairs.h"
#include "board.h"
#include "row.h"  // Row: the explicit test_naked_pair instantiation at file end
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <cassert>
#include <memory>
#include <utility>

namespace {
// File-local: NP's whitebox hook tests the predicate, not the finding, so nothing
// outside this TU needs to name or downcast NakedPairFinding. print() format:
// "{coord1,coord2}#{value1,value2}".
struct NakedPairFinding : Finding {
    std::pair<Coord, Coord> coords;
    std::pair<Value, Value> values;
    NakedPairFinding(std::pair<Coord, Coord> c, std::pair<Value, Value> v) : coords(c), values(v) { }
    bool same(const NakedPairFinding &o) const { return coords == o.coords && values == o.values; }
    void print(std::ostream &o) const override {
        o << "{" << coords.first << "," << coords.second << "}#{" << values.first << "," << values.second << "}";
    }
};

// Would acting on the pair (c1, c2) with values (v1, v2) actually eliminate a
// candidate somewhere in `set`?
template<class Set>
bool would_act(const Set &set, const Cell &c1, const Cell &c2, const Value &v1, const Value &v2) {
    bool would_act = false;

    // can only act on a set that contains both cells
    if (!set.contains(c1)) return would_act;
    if (!set.contains(c2)) return would_act;

    for (auto const &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == c1 || other_cell == c2) continue;
        if (!other_cell.check(v1) && !other_cell.check(v2)) continue; // no impact on this cell

        would_act = true;
    }
    return would_act;
}

// Find a naked pair for `cell` within `set`, if any, appending it to `out`. Was
// Analyzer::find_naked_pair; the member vector dedup is now a scan of `out`
// (this technique's own bucket, so every entry downcasts to NakedPairFinding).
template<class Set>
bool find_naked_pair(const Cell &cell, const Set &set, FindingList &out) {
    // cell is fixed across the loop, so its candidate pair is too; enumerate once.
    auto cellv = cell.notes().values();

    for (auto const &pair_cell : set) {
        // is this candidate pair cell good?
        if (!NakedPairTechnique::test_naked_pair(cell, pair_cell, set)) continue;

        // yes! but is it already recorded?
        NakedPairFinding np({cell.coord(), pair_cell.coord()}, {cellv.at(0), cellv.at(1)});
        bool already = false;
        for (auto const &f : out) {
            if (bucket_cast<NakedPairFinding>(*f).same(np)) { already = true; break; }
        }
        if (already) continue;

        // no! let's record it
        if (sVerbose) { std::cout << "  [fNP] "; np.print(std::cout); std::cout << std::endl; }
        out.push_back(std::make_shared<NakedPairFinding>(np));
        return true;
    }

    return false;
}

// Apply one recorded pair to one `set`, eliminating its two values from the other
// cells. The pair's cells are addressed by coord (findings carry coords, and Cell
// equality / set membership are coord-based anyway) rather than resolved through
// the friends-only Board::at.
template<class Set>
bool act_on_naked_pair(Board &board, const NakedPairFinding &entry, const Set &set) {
    bool did_act = false;

    const Coord &c1 = entry.coords.first;
    const Coord &c2 = entry.coords.second;

    // this is not the set to act on unless it also contains the pair's second
    // cell (was set.contains(cell2); contains compares by coord).
    bool contains_c2 = false;
    for (auto const &cell : set) if (cell.coord() == c2) { contains_c2 = true; break; }
    if (!contains_c2) return did_act;

    for (auto const &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell.coord() == c1 || other_cell.coord() == c2) continue; // not looking at either of the cell pairs

        if (other_cell.check(entry.values.first)) {
            board.clear_note_at(other_cell.coord(), entry.values.first);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.first << " [" << tag(set.kind()) << "]" << std::endl;
            did_act = true;
        }
        if (other_cell.check(entry.values.second)) {
            board.clear_note_at(other_cell.coord(), entry.values.second);
            std::cout << "[NP] " << other_cell.coord() << " x" << entry.values.second << " [" << tag(set.kind()) << "]" << std::endl;
            did_act = true;
        }
    }

    return did_act;
}
} // namespace

template<class Set>
bool NakedPairTechnique::test_naked_pair(const Cell &c1, const Cell &c2, const Set &set) {
    // are these two different cells?
    if (c1 == c2) return false;

    // yes! but is c2 "after" c1?
    if (c2.coord() < c1.coord()) return false;

    // yes! but are both cells in the same set?
    if (!set.contains(c1)) return false;
    if (!set.contains(c2)) return false;

    // yes! but are both cells notes?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;

    // yes! but do both cells have only a pair of candidates?
    if (c1.notes().count() != 2) return false;
    if (c2.notes().count() != 2) return false;

    // yes! but are they the same pairs of candidates? Both cells are bivalue
    // (checked above), so identical candidate sets is one bitmask compare.
    if (c1.notes() != c2.notes()) return false;

    auto values = c1.notes().values();
    auto v1 = values.at(0);
    auto v2 = values.at(1);

    // yes! but would acting on them have an effet?
    if (!would_act(set, c1, c2, v1, v2)) return false;

    // yes!
    return true;
}

// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
// When n candidates are possible in a certain set of n cells all in the same block, row,
// or column, and no other candidates are possible in those cells, then those n candidates
// are not possible elsewhere in that same block, row, or column.
// Applied for n = 2
bool NakedPairTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;
        // yes! but does it have only two notes?
        if (cell.notes().count() != 2) continue;

        // yes! let's see if we can find it a pair?
        did_find |= find_naked_pair(cell, board.row(cell), out);
        did_find |= find_naked_pair(cell, board.column(cell), out);
        did_find |= find_naked_pair(cell, board.nonet(cell), out);
    }

    return did_find;
}

bool NakedPairTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;
    for (auto const &f : mine) {
        auto const &np = bucket_cast<NakedPairFinding>(*f);

        // three units of the pair's first cell, addressed by coord
        did_act |= act_on_naked_pair(board, np, board.row(np.coords.first));
        did_act |= act_on_naked_pair(board, np, board.column(np.coords.first));
        did_act |= act_on_naked_pair(board, np, board.nonet(np.coords.first));
    }
    mine.clear();

    assert(did_act);
    return did_act;
}

// Explicit instantiation so the whitebox test (tests/unit/test_analyzer.cpp) can
// link test_naked_pair<Row> directly. Its only in-TU caller is find_naked_pair,
// which at -O3 g++ inlines, emitting no out-of-line copy of the predicate; the
// external reference from the test TU then fails to link (clang happens to keep a
// weak definition, so it only bit the gcc build). See docs/test-predicate-idiom.md.
template bool NakedPairTechnique::test_naked_pair<Row>(const Cell &, const Cell &, const Row &);
