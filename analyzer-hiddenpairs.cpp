// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-hiddenpairs.h"
#include "board.h"
#include "row.h"  // Row: the explicit test_hidden_pair instantiation at file end
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <cassert>
#include <memory>
#include <utility>

namespace {
// File-local: HP's whitebox hook tests the predicate, not the finding, so nothing
// outside this TU needs to name or downcast HiddenPairFinding. print() emits the
// same bytes the old free operator<<(ostream, Analyzer::HiddenPair) did:
// "{coord1,coord2}#{value1,value2}".
struct HiddenPairFinding : Finding {
    std::pair<Coord, Coord> coords;
    std::pair<Value, Value> values;
    HiddenPairFinding(std::pair<Coord, Coord> c, std::pair<Value, Value> v) : coords(c), values(v) { }
    bool same(const HiddenPairFinding &o) const { return coords == o.coords && values == o.values; }
    void print(std::ostream &o) const override {
        o << "{" << coords.first << "," << coords.second << "}#{" << values.first << "," << values.second << "}";
    }
};

// Resolve a coord to its cell without Board::at (friends-only, dropped when the
// technique left Analyzer -- see the roadmap note). The cell's own row is the
// cheapest public unit that contains it; scan it for the matching coord.
const Cell &cell_at(const Board &board, const Coord &coord) {
    for (auto const &cell : board.row(coord))
        if (cell.coord() == coord) return cell;
    assert(false);  // a coord always lies in its own row
    return board.cells().front();
}

// Find a hidden pair for (cell, v1, v2) within `set`, if any, appending it to
// `out`. Was Analyzer::find_hidden_pair; the member vector dedup is now a scan of
// `out` (this technique's own bucket, so every entry downcasts to HiddenPairFinding).
template<class Set>
bool find_hidden_pair(const Cell &cell, const Value &v1, const Value &v2, const Set &set, FindingList &out) {
    for (auto const &other_cell : set) {
        // is this candidate hidden pair good?
        if (!HiddenPairTechnique::test_hidden_pair(cell, other_cell, v1, v2, set)) continue;

        // yes! but is it already recorded?
        HiddenPairFinding hp({cell.coord(), other_cell.coord()}, {v1, v2});
        bool already = false;
        for (auto const &f : out) {
            assert(dynamic_cast<const HiddenPairFinding *>(f.get()));
            if (static_cast<const HiddenPairFinding *>(f.get())->same(hp)) { already = true; break; }
        }
        if (already) continue;

        // no! let's record it
        if (sVerbose) { std::cout << "  [fHP] "; hp.print(std::cout); std::cout << std::endl; }
        out.push_back(std::make_shared<HiddenPairFinding>(hp));
        return true;
    }

    return false;
}

// Strip every candidate other than the hidden pair's two values from `cell`. Was
// Analyzer::act_on_hidden_pair(cell, entry); the board is now the passed reference
// (the member reached it through Analyzer's mBoard).
bool act_on_hidden_pair(Board &board, const Cell &cell, const HiddenPairFinding &entry) {
    bool did_act = false;

    auto const &v1 = entry.values.first;
    auto const &v2 = entry.values.second;

    for (auto const &value : cell.notes().values()) {
        if (value == v1) continue;
        if (value == v2) continue;

        board.clear_note_at(cell.coord(), value);
        std::cout << "[HP] " << cell.coord() << " x" << value << " "; entry.print(std::cout); std::cout << std::endl;
        did_act = true;
    }

    return did_act;
}
} // namespace

template<class Set>
bool HiddenPairTechnique::test_hidden_pair(const Cell &c1, const Cell &c2,
                                           const Value &v1, const Value &v2, const Set &set) {
    // are these two different cells, carrying two different values?
    if (c1 == c2) return false;
    if (v1 == v2) return false;

    // yes! but is v2 strictly "after" v1? (find_ enumerates v1<v2; a direct test
    // caller can pass them either way, so the predicate guards the precondition
    // itself. The v1==v2 case above is the other half of that guard.)
    if (v2 < v1) return false;

    // yes! but is c2 "after" c1?
    if (c2.coord() < c1.coord()) return false;

    // yes! but are both cells in the same set?
    if (!set.contains(c1)) return false;
    if (!set.contains(c2)) return false;

    // yes! but are both cells notes?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;

    // yes! but do both cells carry the candidate pair?
    if (!c1.check(v1) || !c1.check(v2)) return false;
    if (!c2.check(v1) || !c2.check(v2)) return false;

    // yes! but is the pair "hidden", i.e. no other cell in the set carries
    // either value? (A stray cell carrying exactly one value, or a third cell
    // carrying both, are equally disqualifying -- this one test subsumes both
    // rejection paths the old discovery scan handled separately.)
    for (auto const &other_cell : set) {
        if (!other_cell.isNote()) continue;
        if (other_cell == c1 || other_cell == c2) continue;
        if (other_cell.check(v1) || other_cell.check(v2)) return false;
    }

    // yes! but is it actionable (i.e. *not* a naked pair, with nothing else to strip)?
    if (c1.notes().count() == 2 && c2.notes().count() == 2) return false;

    // yes!
    return true;
}

// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
// When n candidates are possible in a certain set of n cells all in the same block, row, or column,
// and those n candidates are not possible elsewhere in that same block, row, or column, then no other
// candidates are possible in those cells.
// Applied for n = 2
bool HiddenPairTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! but does it have at least two candidate values?
        auto values = cell.notes().values();
        if (values.size() < 2) continue;

        // for each pair of candidates for this cell...
        for (auto pv1 = values.begin(); pv1 != values.end(); ++pv1) {
            for (auto pv2 = pv1 + 1; pv2 != values.end(); ++pv2) {
                assert(*pv2 != *pv1);

                // let's see if we can find a hidden pair in the three cell sets
                did_find |= find_hidden_pair(cell, *pv1, *pv2, board.row(cell), out);
                did_find |= find_hidden_pair(cell, *pv1, *pv2, board.column(cell), out);
                did_find |= find_hidden_pair(cell, *pv1, *pv2, board.nonet(cell), out);
            }
        }
    }

    return did_find;
}

bool HiddenPairTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;
    for (auto const &f : mine) {
        // Bucket invariant: see NakedSingleTechnique::apply for the rationale.
        // The assert turns a wrong-bucket wiring bug into a caught error, not UB.
        assert(dynamic_cast<const HiddenPairFinding *>(f.get()));
        auto const *hp = static_cast<const HiddenPairFinding *>(f.get());

        // the pair's two cells, addressed by coord (findings carry coords)
        did_act |= act_on_hidden_pair(board, cell_at(board, hp->coords.first), *hp);
        did_act |= act_on_hidden_pair(board, cell_at(board, hp->coords.second), *hp);
    }
    mine.clear();

    assert(did_act);
    return did_act;
}

// Explicit instantiation so the whitebox test (tests/unit/test_analyzer.cpp) can
// link test_hidden_pair<Row> directly. Its only in-TU caller is find_hidden_pair,
// which at -O3 g++ inlines, emitting no out-of-line copy of the predicate; the
// external reference from the test TU then fails to link (clang happens to keep a
// weak definition, so it only bit the gcc build). See docs/test-predicate-idiom.md.
template bool HiddenPairTechnique::test_hidden_pair<Row>(const Cell &, const Cell &, const Value &, const Value &, const Row &);
