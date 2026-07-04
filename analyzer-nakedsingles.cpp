// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-nakedsingles.h"
#include "board.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <cassert>
#include <memory>

namespace {
// File-local: Naked Singles has no whitebox hooks, so nothing outside this TU
// needs to name or downcast the finding. print() emits the same bytes the old
// free operator<<(ostream, Analyzer::NakedSingle) did: "coord#value".
struct NakedSingleFinding : Finding {
    Coord coord;
    Value value;
    NakedSingleFinding(const Coord &c, const Value &v) : coord(c), value(v) { }
    void print(std::ostream &o) const override { o << coord << "#" << value; }
};
} // namespace

// A naked single arises when there is only one possible candidate for a cell.
// https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#scanning
bool NakedSingleTechnique::find(const Board &board, FindingList &out) const {
    bool did_find = false;

    for (auto const &cell : board.cells()) {
        // is this a naked single?
        if (!cell.isNote() || cell.notes().count() != 1) continue;

        // yes! let's record it. (No duplicate-coord assert: the bucket is
        // cleared each analyze() and every cell has a distinct coord.)
        auto finding = std::make_shared<NakedSingleFinding>(cell.coord(), cell.notes().values().at(0));
        if (sVerbose) { std::cout << "  [fNS] "; finding->print(std::cout); std::cout << std::endl; }
        out.push_back(std::move(finding));
        did_find = true;
    }

    return did_find;
}

bool NakedSingleTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;

    // singles can be acted on all at once
    for (auto const &f : mine) {
        // Bucket invariant: analyze() routes reg[i]->find() into mFindings[i],
        // so this bucket only ever holds this technique's own findings -- which
        // is what makes the static_cast sound. Nothing but discipline enforces
        // that, and this line is the template every ported apply() copies, so
        // assert the contract to turn a wrong-bucket wiring bug into a caught
        // error instead of UB.
        assert(dynamic_cast<const NakedSingleFinding *>(f.get()));
        auto const *ns = static_cast<const NakedSingleFinding *>(f.get());
        std::cout << "[NS] " << ns->coord << " =" << ns->value << std::endl;
        board.set_value_at(ns->coord, ns->value);
    }
    mine.clear();
    return true;
}
