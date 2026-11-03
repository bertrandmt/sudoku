// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-hiddentriples.h"
#include "board.h"
#include "row.h"  // Row: the explicit test_hidden_triple instantiation at file end
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <array>
#include <cassert>
#include <memory>
#include <vector>

namespace {
// File-local: HT's whitebox hook tests the predicate, not the finding, so nothing
// outside this TU needs to name or downcast HiddenTripleFinding. print() format:
// "{coord1,coord2,coord3}#{value1,value2,value3}[units]", where units is every unit
// in which test_hidden_triple accepts the triple (see UnitSet in board.h for the one
// rule), i.e. where no other cell carries any of the three values. That is not the
// units holding all three cells: cells on a line in one nonet can be hidden in the
// nonet and not the line, or the reverse. apply() does not look at it, since it
// strips the three cells whichever unit made the triple hidden.
struct HiddenTripleFinding : Finding {
    std::array<Coord, 3> coords;
    std::array<Value, 3> values;
    UnitSet units;
    HiddenTripleFinding(std::array<Coord, 3> c, std::array<Value, 3> v, Unit u) : coords(c), values(v) { units.add(u); }
    bool same(const HiddenTripleFinding &o) const { return coords == o.coords && values == o.values; }
    void print(std::ostream &o) const override {
        o << "{" << coords[0] << "," << coords[1] << "," << coords[2] << "}#{"
          << values[0] << "," << values[1] << "," << values[2] << "}" << units;
    }
};

// The values that sit in two or three note cells of `set`: the only ones a hidden
// triple in `set` can be made of. The predicate's own gates imply the bound; it is
// here so that find() skips a value, and with it every triple of values that cannot
// qualify, before pairing any cells.
template<class Set>
Notes triple_values(const Set &set) {
    std::array<size_t, 10> places {};
    for (auto const &cell : set) {
        if (!cell.isNote()) continue;
        for (Value v : cell.notes().values()) places[v]++;
    }

    Notes eligible;
    eligible.clear();
    for (Value v : value_range()) if (places[v] == 2 || places[v] == 3) eligible.set(v, true);
    return eligible;
}

// Find every hidden triple on `values` (ascending, `triple` being the same values as
// a set) whose first cell is `cell` within `set`, merging each into `found`. The
// predicate wants ascending coords, so a triple is reached from its lowest cell only.
// `found` holds what the caller has gathered for this one (cell, values) across its
// units, so a triple hidden in two units is one finding that names both, and the
// caller records it once all three units have been through.
template<class Set>
void find_hidden_triple(const Cell &cell, const std::array<Value, 3> &values, const Notes &triple, const Set &set,
                        std::vector<HiddenTripleFinding> &found) {
    for (auto const &second : set) {
        // the other two cells carry the values too, and come after `cell`
        if (!(cell.coord() < second.coord())) continue;
        if (!second.isNote() || !second.notes().intersects(triple)) continue;

        for (auto const &third : set) {
            if (!HiddenTripleTechnique::test_hidden_triple(cell, second, third, values[0], values[1], values[2], set)) continue;

            // yes! but have we met it through another unit already?
            HiddenTripleFinding ht({cell.coord(), second.coord(), third.coord()}, values, set.kind());
            bool already = false;
            for (auto &f : found) {
                if (f.same(ht)) { f.units.add(set.kind()); already = true; break; }
            }
            if (already) continue;

            // no! let's keep it
            found.push_back(ht);
        }
    }
}

// Strip every candidate other than the hidden triple's three values from the cell at
// `coord`. Coord-native like Hidden Pairs (no coord->Cell resolution).
// clear_note_at is a no-op returning false for an absent note (board.cpp), so
// enumerating value_range() and acting on its return strips exactly the
// candidates the cell actually holds, in ascending order.
bool act_on_hidden_triple(Board &board, const Coord &coord, const HiddenTripleFinding &entry) {
    bool did_act = false;

    for (Value value : value_range()) {
        if (value == entry.values[0] || value == entry.values[1] || value == entry.values[2]) continue;

        if (!board.clear_note_at(coord, value)) continue;  // absent note -> no-op
        std::cout << "[HT] " << coord << " x" << value << " "; entry.print(std::cout); std::cout << std::endl;
        did_act = true;
    }

    return did_act;
}
} // namespace

template<class Set>
bool HiddenTripleTechnique::test_hidden_triple(const Cell &c1, const Cell &c2, const Cell &c3,
                                               const Value &v1, const Value &v2, const Value &v3, const Set &set) {
    // are the cells passed in ascending coord order, and the values in ascending
    // order? (Strict order on each also makes them distinct. find_ guarantees both;
    // a direct test caller can violate either, so the predicate guards them itself.)
    if (!(c1.coord() < c2.coord())) return false;
    if (!(c2.coord() < c3.coord())) return false;
    if (!(v1 < v2)) return false;
    if (!(v2 < v3)) return false;

    // yes! but are all three cells in the same set?
    if (!set.contains(c1)) return false;
    if (!set.contains(c2)) return false;
    if (!set.contains(c3)) return false;

    // yes! but are all three cells notes?
    if (!c1.isNote()) return false;
    if (!c2.isNote()) return false;
    if (!c3.isNote()) return false;

    // yes! but does each value sit in at least two of the three cells? A value in
    // only one is a hidden single, not part of a triple, and one in none is already
    // placed in the set (or a contradiction).
    for (Value v : {v1, v2, v3}) {
        if (static_cast<int>(c1.check(v)) + static_cast<int>(c2.check(v)) + static_cast<int>(c3.check(v)) < 2) return false;
    }

    // yes! but does each cell carry at least one of the values? Three values in two
    // cells would be a contradiction, not a triple.
    Notes triple;
    triple.clear();
    for (Value v : {v1, v2, v3}) triple.set(v, true);
    if (!c1.notes().intersects(triple)) return false;
    if (!c2.notes().intersects(triple)) return false;
    if (!c3.notes().intersects(triple)) return false;

    // yes! but is the triple "hidden", i.e. no other cell in the set carries any of
    // the three values?
    for (auto const &other_cell : set) {
        if (!other_cell.isNote()) continue;
        if (other_cell == c1 || other_cell == c2 || other_cell == c3) continue;
        if (other_cell.notes().intersects(triple)) return false;
    }

    // yes! but is it actionable (i.e. *not* a naked triple, with nothing else to strip)?
    if (c1.notes().subset_of(triple) && c2.notes().subset_of(triple) && c3.notes().subset_of(triple)) return false;

    // yes!
    return true;
}

// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#subsets
// When n candidates are possible in a certain set of n cells all in the same block, row, or column,
// and those n candidates are not possible elsewhere in that same block, row, or column, then no other
// candidates are possible in those cells.
// Applied for n = 3
bool HiddenTripleTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // yes! which values could make a triple in each of its three cell sets?
        auto const &row = board.row(cell);
        auto const &column = board.column(cell);
        auto const &nonet = board.nonet(cell);
        Notes in_row = triple_values(row), in_column = triple_values(column), in_nonet = triple_values(nonet);
        auto pool = in_row.united(in_column).united(in_nonet).values();

        // for each triple of those values (the anchor carries at least one of them)...
        for (size_t i = 0; i < pool.size(); ++i) {
            for (size_t j = i + 1; j < pool.size(); ++j) {
                for (size_t k = j + 1; k < pool.size(); ++k) {
                    std::array<Value, 3> values = {pool[i], pool[j], pool[k]};
                    Notes triple;
                    triple.clear();
                    for (Value v : values) triple.set(v, true);
                    if (!cell.notes().intersects(triple)) continue;

                    // let's see if we can find a hidden triple in the three cell sets
                    std::vector<HiddenTripleFinding> found;
                    if (triple.subset_of(in_row))    find_hidden_triple(cell, values, triple, row, found);
                    if (triple.subset_of(in_column)) find_hidden_triple(cell, values, triple, column, found);
                    if (triple.subset_of(in_nonet))  find_hidden_triple(cell, values, triple, nonet, found);

                    // A triple is reached from its first cell only (the predicate wants
                    // ascending coords) and `found` is per (cell, values), so nothing
                    // here can already be in `out`: no dedup scan, just record.
                    for (auto const &ht : found) {
                        if (sVerbose) { std::cout << "  [fHT] "; ht.print(std::cout); std::cout << std::endl; }
                        out.push_back(std::make_shared<HiddenTripleFinding>(ht));
                        did_find = true;
                    }
                }
            }
        }
    }

    return did_find;
}

bool HiddenTripleTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;
    for (auto const &f : mine) {
        auto const &ht = bucket_cast<HiddenTripleFinding>(*f);

        // the triple's three cells, addressed by coord (findings carry coords)
        for (auto const &coord : ht.coords) did_act |= act_on_hidden_triple(board, coord, ht);
    }
    mine.clear();

    assert(did_act);
    return did_act;
}

// Explicit instantiation so the whitebox test (tests/unit/test_analyzer.cpp) can
// link test_hidden_triple<Row> directly. Its only in-TU caller is find_hidden_triple,
// which at -O3 g++ inlines, emitting no out-of-line copy of the predicate; the
// external reference from the test TU then fails to link (clang happens to keep a
// weak definition, so it only bit the gcc build). See docs/test-predicate-idiom.md.
template bool HiddenTripleTechnique::test_hidden_triple<Row>(const Cell &, const Cell &, const Cell &,
                                                             const Value &, const Value &, const Value &, const Row &);
