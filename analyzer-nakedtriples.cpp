// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-nakedtriples.h"
#include "board.h"
#include "row.h"  // Row: the explicit test_naked_triple instantiation at file end
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <array>
#include <cassert>
#include <memory>
#include <vector>

namespace {
// File-local: NT's whitebox hook tests the predicate, not the finding, so nothing
// outside this TU needs to name or downcast NakedTripleFinding. print() format:
// "{coord1,coord2,coord3}#{value1,value2,value3}[units]", where units is every unit
// in which test_naked_triple accepts the triple (see UnitSet in board.h for the one
// rule). That is the units that hold all three cells and have something to strike.
// apply() does not look at it: it strikes in every unit holding all three cells,
// and a unit the line leaves out has nothing to strike.
struct NakedTripleFinding : Finding {
    std::array<Coord, 3> coords;
    std::array<Value, 3> values;
    UnitSet units;
    NakedTripleFinding(std::array<Coord, 3> c, std::array<Value, 3> v, Unit u) : coords(c), values(v) { units.add(u); }
    bool same(const NakedTripleFinding &o) const { return coords == o.coords && values == o.values; }
    void print(std::ostream &o) const override {
        o << "{" << coords[0] << "," << coords[1] << "," << coords[2] << "}#{"
          << values[0] << "," << values[1] << "," << values[2] << "}" << units;
    }
};

// A cell that can be part of a naked triple holds two or three candidates. A
// single is a naked single's business, and four or more cannot fit in three values.
// The upper bound is implied by the span gate in test_naked_triple; it is here so
// find() skips such cells before pairing them with anything.
bool triple_sized(const Cell &c) {
    return c.isNote() && c.notes().count() >= 2 && c.notes().count() <= 3;
}

// Would acting on the triple (c1, c2, c3) with the three values `span` actually
// eliminate a candidate somewhere in `set`?
template<class Set>
bool would_act(const Set &set, const Cell &c1, const Cell &c2, const Cell &c3, const Notes &span) {
    for (auto const &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell == c1 || other_cell == c2 || other_cell == c3) continue;
        if (other_cell.notes().intersects(span)) return true;
    }
    return false;
}

// Find every naked triple whose first cell is `cell` within `set`, merging each
// into `found`. The predicate wants ascending coords, so a triple is reached from
// its lowest cell only. `found` holds what the caller has gathered for this one
// `cell` across its units, so a triple accepted in two units is one finding that
// names both, and the caller records it once all three units have been through.
template<class Set>
bool find_naked_triple(const Cell &cell, const Set &set, std::vector<NakedTripleFinding> &found) {
    bool did_find = false;

    for (auto const &second : set) {
        // prune: two cells already spanning more than three values cannot be part
        // of any triple, whatever the third cell is
        if (!(cell.coord() < second.coord())) continue;
        if (!triple_sized(second)) continue;
        if (cell.notes().united(second.notes()).count() > 3) continue;

        for (auto const &third : set) {
            if (!NakedTripleTechnique::test_naked_triple(cell, second, third, set)) continue;

            auto span = cell.notes().united(second.notes()).united(third.notes()).values();
            NakedTripleFinding nt({cell.coord(), second.coord(), third.coord()},
                                  {span.at(0), span.at(1), span.at(2)}, set.kind());
            bool already = false;
            for (auto &f : found) {
                if (f.same(nt)) { f.units.add(set.kind()); already = true; break; }
            }
            if (already) continue;

            found.push_back(nt);
            did_find = true;
        }
    }

    return did_find;
}

// Apply one recorded triple to one `set`, eliminating its three values from the
// other cells. The triple's cells are addressed by coord, as in Naked Pairs.
template<class Set>
bool act_on_naked_triple(Board &board, const NakedTripleFinding &entry, const Set &set) {
    bool did_act = false;

    // this is not the set to act on unless it holds all three of the triple's cells
    for (auto const &c : entry.coords) {
        bool contains = false;
        for (auto const &cell : set) if (cell.coord() == c) { contains = true; break; }
        if (!contains) return did_act;
    }

    for (auto const &other_cell : set) {
        if (other_cell.isValue()) continue;
        if (other_cell.coord() == entry.coords[0]
         || other_cell.coord() == entry.coords[1]
         || other_cell.coord() == entry.coords[2]) continue; // not looking at the triple's own cells

        for (auto const &v : entry.values) {
            if (!other_cell.check(v)) continue;
            board.clear_note_at(other_cell.coord(), v);
            std::cout << "[NT] " << other_cell.coord() << " x" << v << " [" << tag(set.kind()) << "]" << std::endl;
            did_act = true;
        }
    }

    return did_act;
}
} // namespace

template<class Set>
bool NakedTripleTechnique::test_naked_triple(const Cell &c1, const Cell &c2, const Cell &c3, const Set &set) {
    // are the three cells distinct, and passed in ascending coord order? (Strict
    // order on all three also makes them distinct.)
    if (!(c1.coord() < c2.coord())) return false;
    if (!(c2.coord() < c3.coord())) return false;

    // yes! but are all three cells in the same set?
    if (!set.contains(c1)) return false;
    if (!set.contains(c2)) return false;
    if (!set.contains(c3)) return false;

    // yes! but are all three cells notes holding two or three candidates?
    if (!triple_sized(c1)) return false;
    if (!triple_sized(c2)) return false;
    if (!triple_sized(c3)) return false;

    // yes! but do they together span exactly three values? Fewer than three would
    // be a contradiction (three cells, too few values), and more is no triple.
    auto span = c1.notes().united(c2.notes()).united(c3.notes());
    if (span.count() != 3) return false;

    // yes! but would acting on them have an effect?
    if (!would_act(set, c1, c2, c3, span)) return false;

    // yes!
    return true;
}

// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
// When n candidates are possible in a certain set of n cells all in the same block, row,
// or column, and no other candidates are possible in those cells, then those n candidates
// are not possible elsewhere in that same block, row, or column.
// Applied for n = 3
bool NakedTripleTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (auto const &cell: board.cells()) {
        // can this note cell start a triple?
        if (!triple_sized(cell)) continue;

        std::vector<NakedTripleFinding> found;
        find_naked_triple(cell, board.row(cell), found);
        find_naked_triple(cell, board.column(cell), found);
        find_naked_triple(cell, board.nonet(cell), found);

        // A triple is reached from its lowest cell only and `found` is per cell,
        // so nothing here can already be in `out`: no dedup scan, just record.
        for (auto const &nt : found) {
            if (sVerbose) { std::cout << "  [fNT] "; nt.print(std::cout); std::cout << std::endl; }
            out.push_back(std::make_shared<NakedTripleFinding>(nt));
            did_find = true;
        }
    }

    return did_find;
}

bool NakedTripleTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;
    for (auto const &f : mine) {
        auto const &nt = bucket_cast<NakedTripleFinding>(*f);

        // three units of the triple's first cell, addressed by coord
        did_act |= act_on_naked_triple(board, nt, board.row(nt.coords[0]));
        did_act |= act_on_naked_triple(board, nt, board.column(nt.coords[0]));
        did_act |= act_on_naked_triple(board, nt, board.nonet(nt.coords[0]));
    }
    mine.clear();

    assert(did_act);
    return did_act;
}

// Explicit instantiation so the whitebox test (tests/unit/test_analyzer.cpp) can
// link test_naked_triple<Row> directly. Its only in-TU caller is find_naked_triple,
// which at -O3 g++ inlines, emitting no out-of-line copy of the predicate; the
// external reference from the test TU then fails to link (clang happens to keep a
// weak definition, so it only bit the gcc build). See docs/test-predicate-idiom.md.
template bool NakedTripleTechnique::test_naked_triple<Row>(const Cell &, const Cell &, const Cell &, const Row &);
