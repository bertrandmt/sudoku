// Copyright (c) 2025, Bertrand Mollivier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>
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

            std::string tag;
            if (board.see_each_other(cell.coord(), wing2.coord(), tag)) {
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
}

bool Analyzer::test_ywing(const Cell &pivot, const Cell &wing1, const Cell &wing2, Value &out_value) const {
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

    assert(mBoard.see_each_other(pivot, wing1));
    assert(mBoard.see_each_other(pivot, wing2));

    // does wing1 share only one value with pivot (and which is it?)
    Value wing1_shared = kUnset;
    if (pivot.check(wing1.notes().values()[0])) {
        if (pivot.check(wing1.notes().values()[1])) return false;
        wing1_shared = wing1.notes().values()[0];
    } else {
        if (pivot.check(wing1.notes().values()[0])) return false;
        wing1_shared = wing1.notes().values()[1];
    }

    // yes! but does wing2 share only one value with pivot (and which is it?)
    Value wing2_shared = kUnset;
    if (pivot.check(wing2.notes().values()[0])) {
        if (pivot.check(wing2.notes().values()[1])) return false;
        wing2_shared = wing2.notes().values()[0];
    } else {
        if (pivot.check(wing2.notes().values()[0])) return false;
        wing2_shared = wing2.notes().values()[1];
    }

    // yes! but do wing1 and wing2 share different values with pivot?
    if (wing1_shared == wing2_shared) return false;

    // Find the elimination candidate (the candidate that both wings have but pivot doesn't)
    Value wing1_other = wing1_shared == wing1.notes().values()[0] ? wing1.notes().values()[1] : wing1.notes().values()[0];
    Value wing2_other = wing2_shared == wing2.notes().values()[0] ? wing2.notes().values()[1] : wing2.notes().values()[0];
    if (wing1_other != wing2_other) return false;

    out_value = wing1_other;

    if (!would_act(mBoard, pivot, wing1, wing2, out_value)) return false;

    return true;
}

namespace {
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
            if (!pivot.check(cell.notes().values()[0]) && !pivot.check(cell.notes().values()[1])) continue;

            // yes! but are both candidates not candidates for pivot?
            if (pivot.notes().values() == cell.notes().values()) continue;

            // yes! ok, this is a bona fide wing candidate; record it
            // unordered_set automatically handles duplicates
            wing_candidates.insert(cell);
        }
    }
}

bool Analyzer::find_ywing(const Cell &pivot) {
    assert(pivot.isNote());
    assert(pivot.notes().count() == 2);

    bool did_find = false;

    // Get all cells that the pivot can see
    std::unordered_set<Cell> wing_candidates;
    select_wing_candidates(pivot, mBoard.row(pivot), wing_candidates);
    select_wing_candidates(pivot, mBoard.column(pivot), wing_candidates);
    select_wing_candidates(pivot, mBoard.nonet(pivot), wing_candidates);

    // Try all pairs of visible cells as potential wings
    for (auto it1 = wing_candidates.begin(); it1 != wing_candidates.end(); ++it1) {
        for (auto it2 = std::next(it1); it2 != wing_candidates.end(); ++it2) {
            const Cell &wing1 = *it1;
            const Cell &wing2 = *it2;

            Value ywing_value = kUnset;
            if (!test_ywing(pivot, wing1, wing2, ywing_value)) continue;
            assert(wing1.check(ywing_value));
            assert(wing2.check(ywing_value));

            // Record the Y-Wing pattern
            YWing yw{ywing_value, pivot.coord(), {wing1.coord(), wing2.coord()}};
            if (std::find(mYWings.begin(), mYWings.end(), yw) != mYWings.end()) continue;

            mYWings.push_back(yw);
            if (sVerbose) std::cout << "  [fYW] " << yw << std::endl;
            did_find = true;
        }
    }

    return did_find;
}

bool Analyzer::find_ywings() {
    // https://www.sudokuwiki.org/Y_Wing_Strategy
    // A Y-Wing consists of three cells, each with exactly two candidates:
    // - One pivot cell with candidates AB
    // - One wing cell sharing A with pivot (has candidates AC)
    // - One wing cell sharing B with pivot (has candidates BC)
    // The pivot can see both wings, but wings don't need to see each other
    // Any cell that can see both wings can have candidate C eliminated

    assert(mYWings.empty());
    bool did_find = false;

    for (auto const &cell : mBoard.cells()) {
        // Is this a note cell with exactly 2 candidates?
        if (!cell.isNote()) continue;
        if (cell.notes().count() != 2) continue;

        // Try this cell as a pivot
        did_find |= find_ywing(cell);
    }

    return did_find;
}

template <class Set>
bool Analyzer::act_on_ywing(const YWing &entry, const Set &wing1_set) {
    assert(wing1_set.contains(mBoard.at(entry.wings.first)));

    bool did_act = false;

    for (auto const &cell : wing1_set) {
        if (cell.isValue()) continue;
        if (cell.coord() == entry.pivot) continue;
        if (cell.coord() == entry.wings.first) continue;
        if (cell.coord() == entry.wings.second) continue;
        if (!cell.check(entry.value)) continue;

        std::string tag;
        if (mBoard.see_each_other(cell.coord(), entry.wings.second, tag)) {
            std::cout << "[YW] " << cell.coord() << " x" << entry.value << std::endl;
            mBoard.clear_note_at(cell.coord(), entry.value);
            did_act = true;
        }
    }

    return did_act;
}

bool Analyzer::act_on_ywing() {
    bool did_act = false;

    if (mYWings.empty()) return did_act;

    for (auto const &entry : mYWings) {
        // for each entry, look for candidates for elimination within the first wing's row, column or nonet
        did_act |= act_on_ywing(entry, mBoard.row(entry.wings.first));
        did_act |= act_on_ywing(entry, mBoard.column(entry.wings.first));
        did_act |= act_on_ywing(entry, mBoard.nonet(entry.wings.first));
    }
    mYWings.clear();

    assert(did_act);
    return did_act;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::YWing &yw) {
    outs << yw.pivot << "Y{" << yw.wings.first << "," << yw.wings.second << "}#" << yw.value;
    return outs;
}
