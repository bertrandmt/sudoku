// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-ywing.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <algorithm>
#include <iterator>
#include <cassert>
#include <memory>
#include <optional>
#include <vector>
#include <unordered_set>

namespace { // anon
    template<class Set>
    bool would_act_for_set(const Board &board, const Cell &pivot, const Cell &wing1, const Cell &wing2, const Value &value, const Set &wing1_set) {
        assert(wing1_set.contains(wing1));

        bool would_act = false;

        for (auto const &cell : wing1_set) {
            if (cell.isValue()) continue;
            if (cell.coord() == pivot.coord()) continue;
            if (cell.coord() == wing1.coord()) continue;
            if (cell.coord() == wing2.coord()) continue;
            if (!cell.check(value)) continue;

            if (board.see_each_other(cell.coord(), wing2.coord())) {
                would_act = true;
                break;
            }
        }
        return would_act;
    }

    bool would_act(const Board &board, const Cell &pivot, const Cell &wing1, const Cell &wing2, const Value &value) {
        bool would_act = false;

        would_act = would_act_for_set(board, pivot, wing1, wing2, value, board.row(wing1));
        if (!would_act) would_act = would_act_for_set(board, pivot, wing1, wing2, value, board.column(wing1));
        if (!would_act) would_act = would_act_for_set(board, pivot, wing1, wing2, value, board.nonet(wing1));

        return would_act;
    }

    template<class Set>
    void select_wing_candidates(const Cell &pivot, const Set &set, std::unordered_set<Cell> &wing_candidates) {
        assert(set.contains(pivot));

        for (auto const &cell : set) {
            // is this another cell than the pivot?
            if (cell == pivot) continue;

            // yes! but is it a note?
            if (!cell.isNote()) continue;

            // yes! but does it have only two candidates?
            if (cell.notes().count() != 2) continue;

            // yes! but is one of the candidates also a candidate for pivot?
            if (!pivot.notes().intersects(cell.notes())) continue;

            // yes! but are both candidates not candidates for pivot? Both cells
            // are bivalue, so identical candidate sets is one bitmask compare.
            if (pivot.notes() == cell.notes()) continue;

            // yes! ok, this is a bona fide wing candidate; record it
            // unordered_set automatically handles duplicates
            wing_candidates.insert(cell);
        }
    }

    // Apply one recorded Y-Wing to one `wing1_set`: clear `value` from every cell
    // of that set that also sees wing2.
    template <class Set>
    bool act_on_ywing(Board &board, const YWingFinding &entry, const Set &wing1_set) {
        // Contract mirror of would_act_for_set's find-side assert: the set we were
        // handed is wing1's own unit. wings.first is a Coord (findings carry coords,
        // and a technique has no friend Board::at to resolve it to a Cell), so
        // check membership by coord.
        assert(std::any_of(wing1_set.begin(), wing1_set.end(),
            [&](const Cell &c) { return c.coord() == entry.wings.first; }));

        bool did_act = false;

        for (auto const &cell : wing1_set) {
            if (cell.isValue()) continue;
            if (cell.coord() == entry.pivot) continue;
            if (cell.coord() == entry.wings.first) continue;
            if (cell.coord() == entry.wings.second) continue;
            if (!cell.check(entry.value)) continue;

            if (board.see_each_other(cell.coord(), entry.wings.second)) {
                std::cout << "[YW] " << cell.coord() << " x" << entry.value << std::endl;
                board.clear_note_at(cell.coord(), entry.value);
                did_act = true;
            }
        }

        return did_act;
    }
} // namespace

bool YWingTechnique::test_ywing(const Board &board, const Cell &pivot, const Cell &wing1, const Cell &wing2, std::optional<Value> &out_value) {
    // by construct, all these assertions apply for input parameters
    assert(pivot != wing1);
    assert(pivot != wing2);
    assert(wing1 != wing2);

    assert(pivot.isNote());
    assert(wing1.isNote());
    assert(wing2.isNote());

    assert(pivot.notes().count() == 2);
    assert(wing1.notes().count() == 2);
    assert(wing2.notes().count() == 2);

    assert(board.see_each_other(pivot, wing1));
    assert(board.see_each_other(pivot, wing2));

    // by construct (select_wing_candidates), each wing shares exactly one value
    // with the pivot: candidates with no value in common with the pivot are
    // skipped (at least one shared), and candidates with the identical 2-value
    // set as the pivot are skipped (no more than one shared).

    // which value does each wing share with pivot? By construction exactly one,
    // so this is the single bit in each wing's intersection with the pivot.
    Value wing1_shared = wing1.notes().shared_value(pivot.notes());
    Value wing2_shared = wing2.notes().shared_value(pivot.notes());

    // yes! but do wing1 and wing2 share different values with pivot?
    if (wing1_shared == wing2_shared) return false;

    // Find the elimination candidate (the candidate that both wings have but pivot doesn't)
    Value wing1_other = wing1.other_value(wing1_shared);
    Value wing2_other = wing2.other_value(wing2_shared);
    if (wing1_other != wing2_other) return false;

    out_value = wing1_other;

    if (!would_act(board, pivot, wing1, wing2, wing1_other)) return false;

    return true;
}

bool YWingTechnique::find_ywing(const Board &board, const Cell &pivot, FindingList &out) {
    assert(pivot.isNote());
    assert(pivot.notes().count() == 2);

    bool did_find = false;

    // Get all cells that the pivot can see
    std::unordered_set<Cell> wing_candidates;
    select_wing_candidates(pivot, board.row(pivot), wing_candidates);
    select_wing_candidates(pivot, board.column(pivot), wing_candidates);
    select_wing_candidates(pivot, board.nonet(pivot), wing_candidates);

    // Enumerate pairs over a coord-sorted view, not over the unordered_set
    // directly. The set's iteration order is unspecified and differs between
    // standard libraries, and it reaches stdout two ways:
    //
    //  - which cell of a pair is wing1 and which is wing2, so it fixes the
    //    *contents* of a printed finding -- "[1, 5]Y{[9, 5],[1, 7]}#9" versus
    //    "[1, 5]Y{[1, 7],[9, 5]}#9" -- not just the order of findings;
    //  - the order findings are pushed, hence their order within "[YW](2) {A, B}"
    //    and the order of the "[YW] ... x<v>" lines apply() emits.
    //
    // The elimination *set* does not depend on any of it: find() records every
    // Y-Wing and apply() acts on all of them, so sorting is free of behavioral
    // cost while still changing printed output on a handful of corpus boards. To see
    // which, *reverse* this comparator rather than deleting the sort: reversal is
    // deterministic on every standard library, while with no sort the order is
    // unspecified and the set you get is your hash table's. The two need not agree,
    // and on this corpus they do not.
    //
    // Reversing it is caught -- run.sh's README Y-Wing worked example compares live
    // output text. Deleting it is caught by nothing: measured, both suites stay green,
    // because unspecified is not the same as wrong. So a green `make test` after
    // deleting this sort is not evidence that the sort is unnecessary. Same asymmetry
    // analyzer-xychain.cpp records for its own sort, except that there the deletion
    // probe does happen to fail, on libc++ only.
    //
    // Counts stated without arithmetic on purpose: the corpus grows with every
    // technique that brings a fixture, so an "N of M" here expires on its own, which
    // is how the previous wording came to cite a corpus two boards smaller than the
    // one it described (#63).
    // (Contrast XY-chain, where discovery order selects *which* chain is kept, so
    // the two are not the same case; see analyzer-xychain.cpp.)
    std::vector<const Cell *> ordered;
    ordered.reserve(wing_candidates.size());
    for (const Cell &c : wing_candidates) ordered.push_back(&c);
    std::sort(ordered.begin(), ordered.end(),
              [](const Cell *a, const Cell *b) { return a->coord() < b->coord(); });

    // Try all pairs of visible cells as potential wings
    for (auto it1 = ordered.begin(); it1 != ordered.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != ordered.end(); ++it2) {
            const Cell &wing1 = **it1;
            const Cell &wing2 = **it2;

            std::optional<Value> ywing_value;
            if (!test_ywing(board, pivot, wing1, wing2, ywing_value)) continue;
            assert(ywing_value.has_value());
            assert(wing1.check(*ywing_value));
            assert(wing2.check(*ywing_value));

            // Record the Y-Wing pattern -- but is it already recorded?
            YWingFinding yw{*ywing_value, pivot.coord(), {wing1.coord(), wing2.coord()}};
            bool already = false;
            for (auto const &f : out) {
                if (bucket_cast<YWingFinding>(*f).same(yw)) { already = true; break; }
            }
            if (already) continue;

            if (sVerbose) { std::cout << "  [fYW] "; yw.print(std::cout); std::cout << std::endl; }
            out.push_back(std::make_shared<YWingFinding>(yw));
            did_find = true;
        }
    }

    return did_find;
}

// https://www.sudokuwiki.org/Y_Wing_Strategy
// A Y-Wing consists of three cells, each with exactly two candidates:
// - One pivot cell with candidates AB
// - One wing cell sharing A with pivot (has candidates AC)
// - One wing cell sharing B with pivot (has candidates BC)
// The pivot can see both wings, but wings don't need to see each other
// Any cell that can see both wings can have candidate C eliminated
bool YWingTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());
    bool did_find = false;

    for (auto const &cell : board.cells()) {
        // Is this a note cell with exactly 2 candidates?
        if (!cell.isNote()) continue;
        if (cell.notes().count() != 2) continue;

        // Try this cell as a pivot
        did_find |= find_ywing(board, cell, out);
    }

    return did_find;
}

bool YWingTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    bool did_act = false;
    for (auto const &f : mine) {
        auto const &yw = bucket_cast<YWingFinding>(*f);

        // for each entry, look for candidates for elimination within the first
        // wing's row, column or nonet
        did_act |= act_on_ywing(board, yw, board.row(yw.wings.first));
        did_act |= act_on_ywing(board, yw, board.column(yw.wings.first));
        did_act |= act_on_ywing(board, yw, board.nonet(yw.wings.first));
    }
    mine.clear();

    assert(did_act);
    return did_act;
}
