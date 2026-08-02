// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "techniques.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "verbose.h"

#include <cassert>
#include <iterator>
#include <memory>
#include <string_view>

// The stateless techniques, built once and shared by every Analyzer. This list
// *is* the solver's cascade: analyze() and act() walk it in order, so adding a
// technique means one new file and one push_back here.
const std::vector<std::unique_ptr<Technique>> &Analyzer::registry() {
    // Canonical cascade order. Order is a correctness property, not a style
    // convention -- the cheapest technique that fires must fire first -- so it is
    // spelled out here once and checked against the built registry below, rather
    // than left implicit in the sequence of push_backs. Through the issue-#7
    // series this list also outran the registry, which was required to be a
    // strict in-order prefix of it while the rest of the cascade was still
    // hand-written; the last port closed that gap, so the two now match exactly.
    [[maybe_unused]] static constexpr const char *kCascade[] = {
        "NS", "HS", "NP", "LC", "HP", "XW", "SC", "YW", "SF", "XY",
    };
    static const std::vector<std::unique_ptr<Technique>> reg = [] {
        std::vector<std::unique_ptr<Technique>> r;
        r.push_back(std::make_unique<NakedSingleTechnique>());
        r.push_back(std::make_unique<HiddenSingleTechnique>());
        r.push_back(std::make_unique<NakedPairTechnique>());
        r.push_back(std::make_unique<LockedCandidatesTechnique>());
        r.push_back(std::make_unique<HiddenPairTechnique>());
        r.push_back(std::make_unique<XWingTechnique>());
        r.push_back(std::make_unique<ColorChainTechnique>());
        r.push_back(std::make_unique<YWingTechnique>());
        r.push_back(std::make_unique<SwordfishTechnique>());
        r.push_back(std::make_unique<XYChainTechnique>());
        assert(r.size() == std::size(kCascade));
        for (size_t i = 0; i < r.size(); ++i)
            assert(std::string_view(r[i]->name()) == kCascade[i]);
        return r;
    }();
    return reg;
}

template<class Set>
void Analyzer::filter_notes(Cell &cell, const Set &set) {
    // note cell; update its own notes from all the value cells in the same set
    if (cell.isNote()) {
        for (auto const &other_cell : set) {
            // is the other cell a value note?
            if (other_cell.isNote()) continue;
            // yes, but is the value of other_cell already checked off cell's notes
            if (!cell.check(other_cell.value())) continue;

            if (sVerbose) std::cout << "  [FNn] " << cell.coord() << " x" << other_cell.value()
                      << " " << tag(set.kind()) << "(" << other_cell.coord() << ")" << std::endl;
            mBoard.clear_note_at(cell.coord(), other_cell.value());
        }
    }
    // value cell; let's update notes in note cells in the same set
    else {
        assert(cell.isValue());
        for (auto &other_cell : set) {
            // is other_cell a note cell?
            if (other_cell.isValue()) continue;
            // yes, but is the value of cell already checked off other_cell's notes
            if (!other_cell.check(cell.value())) continue;

            if (sVerbose) std::cout << "  [FNv] " << other_cell.coord() << " x" << cell.value()
                      << " " << tag(set.kind()) << "(" << cell.coord() << ")" << std::endl;
            mBoard.clear_note_at(other_cell.coord(), cell.value());
        }
    }
}

void Analyzer::filter_notes() {
    for (auto &cell: mBoard.cells()) {
        filter_notes(cell, mBoard.nonet(cell));
        filter_notes(cell, mBoard.column(cell));
        filter_notes(cell, mBoard.row(cell));
    }
}

void Analyzer::analyze() {
    filter_notes();

    for (auto &b : mFindings) b.clear();

    // Walk the cascade in order, each technique finding into its own bucket, and
    // stop at the first one that finds anything: the cheapest firing technique is
    // the one the solver applies.
    const auto &reg = registry();
    // mFindings is indexed in lockstep with reg (bucket i belongs to reg[i]);
    // the two are sized together at construction. Assert it here rather than
    // trust it: this positional coupling is the invariant that replaced the ten
    // hand-synchronized init sites, so it gets the same executable guard.
    assert(mFindings.size() == reg.size());
    bool did_find = false;
    for (size_t i = 0; i < reg.size() && !did_find; ++i)
        did_find = reg[i]->find(mBoard, mFindings[i]);
}

bool Analyzer::act(const bool singles_only) {
    bool did_act = false;

    // Same cascade order as analyze(). Single-tier techniques always run;
    // Advanced ones are skipped under singles_only.
    const auto &reg = registry();
    assert(mFindings.size() == reg.size());  // lockstep index; see analyze()
    for (size_t i = 0; i < reg.size() && !did_act; ++i) {
        if (reg[i]->tier() == Tier::Advanced && singles_only) continue;
        did_act = reg[i]->apply(mBoard, mFindings[i]);
    }

    return did_act;
}

namespace {
// Render one "[TAG](count) {e1, e2, ...}" line of the analyzer dump. The
// singles print their elements bare; every other technique wraps each element
// in its own braces, which is what brace_each() selects. All framing (tag,
// count, separators, per-item braces) lives here, so every technique's line is
// laid out by the same code and only the per-finding body differs.
void print_section(std::ostream &outs, const Technique &tech, const FindingList &findings) {
    outs << "[" << tech.name() << "](" << findings.size() << ") {";
    bool is_first = true;
    for (auto const &finding : findings) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        if (tech.brace_each()) { outs << "{"; finding->print(outs); outs << "}"; }
        else                   {              finding->print(outs);              }
    }
    outs << "}";
}
} // namespace

std::ostream &operator<<(std::ostream &outs, Analyzer const &a) {
    // One line per technique, in cascade order, separated -- not terminated -- by
    // endl. The dump deliberately does not end in a newline: SolverState's
    // operator<< streams the analyzer last and appends nothing. Before the final
    // port that fell out of the shape of the code (the loop terminated each of
    // its lines, and the hand-written XY section after it did not); now it is the
    // loop's own business, hence the separator form.
    const auto &reg = Analyzer::registry();
    assert(a.mFindings.size() == reg.size());  // lockstep index; see analyze()
    for (size_t i = 0; i < reg.size(); ++i) {
        if (i > 0) outs << std::endl;
        print_section(outs, *reg[i], a.mFindings[i]);
    }

    return outs;
}
