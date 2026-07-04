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
#include <stdexcept>
#include <string_view>

// The stateless techniques, built once and shared by every Analyzer. Grows one
// entry per port (cascade order); PR 1 holds only Naked Singles.
const std::vector<std::unique_ptr<Technique>> &Analyzer::registry() {
    // Canonical cascade order for the whole issue-#7 series. The registry must
    // always be a strict in-order PREFIX of this list: analyze()/act() run the
    // registry ahead of the hand-written suffix, so a technique ported out of
    // order would silently reorder application and change solver behavior. That
    // makes cascade order a correctness *precondition* for every PR in the
    // series, not a style convention -- the guard below makes it executable.
    // (It checks the registry side; the suffix stays hand-ordered source that
    // each port shortens by one, and remains a reviewer obligation.)
    [[maybe_unused]] static constexpr const char *kCascade[] = {
        "NS", "HS", "NP", "LC", "HP", "XW", "SC", "YW", "SF", "XY",
    };
    static const std::vector<std::unique_ptr<Technique>> reg = [] {
        std::vector<std::unique_ptr<Technique>> r;
        r.push_back(std::make_unique<NakedSingleTechnique>());
        assert(r.size() <= std::size(kCascade));
        for (size_t i = 0; i < r.size(); ++i)
            assert(std::string_view(r[i]->tag()) == kCascade[i]);
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
    mHiddenSingles.clear();
    mNakedPairs.clear();
    mLockedCandidates.clear();
    mHiddenPairs.clear();
    mXWings.clear();
    mSwordfish.clear();
    mColorChains.clear();
    mYWings.clear();
    mXYChains.clear();

    bool did_find = false;
    // Registry prefix, then the hand-written suffix below: the two must together
    // reproduce the exact cascade order (a correctness precondition -- see the
    // guard in registry()). Each technique finds into its own bucket.
    const auto &reg = registry();
    for (size_t i = 0; i < reg.size() && !did_find; ++i)
        did_find = reg[i]->find(mBoard, mFindings[i]);
    if (!did_find) did_find = find_hidden_singles();
    if (!did_find) did_find = find_naked_pairs();
    if (!did_find) did_find = find_locked_candidates();
    if (!did_find) did_find = find_hidden_pairs();
    if (!did_find) did_find = find_xwings();
    if (!did_find) did_find = find_color_chains();
    if (!did_find) did_find = find_ywings();
    if (!did_find) did_find = find_swordfish();
    if (!did_find) did_find = find_xychains();
}

bool Analyzer::act(const bool singles_only) {
    bool did_act = false;

    // Registry prefix (cascade order). Single-tier techniques always run;
    // Advanced ones are skipped under singles_only, mirroring the suffix gate.
    const auto &reg = registry();
    for (size_t i = 0; i < reg.size() && !did_act; ++i) {
        if (reg[i]->tier() == Tier::Advanced && singles_only) continue;
        did_act = reg[i]->apply(mBoard, mFindings[i]);
    }
    if (!did_act) did_act = act_on_hidden_single();

    if (!singles_only) {
        if (!did_act) did_act = act_on_naked_pair();
        if (!did_act) did_act = act_on_locked_candidate();
        if (!did_act) did_act = act_on_hidden_pair();
        if (!did_act) did_act = act_on_xwing();
        if (!did_act) did_act = act_on_color_chain();
        if (!did_act) did_act = act_on_ywing();
        if (!did_act) did_act = act_on_swordfish();
        if (!did_act) did_act = act_on_xychain();
    }

    return did_act;
}

namespace {
// Render one "[TAG](count) {e1, e2, ...}" line of the analyzer dump. The
// singles print their elements bare; every other technique wraps each element
// in its own braces. All framing (tag, count, separators, per-item braces)
// lives here so the registry-prefix and hand-written-suffix sections share
// every byte; each caller supplies only how to render one item.
template<class Container, class Render>
void print_section_core(std::ostream &outs, const char *tag, const Container &items,
                        bool brace_each, Render render) {
    outs << "[" << tag << "](" << items.size() << ") {";
    bool is_first = true;
    for (auto const &item : items) {
        if (!is_first) { outs << ", "; }
        is_first = false;
        if (brace_each) { outs << "{"; render(outs, item); outs << "}"; }
        else            {              render(outs, item);              }
    }
    outs << "}";
}

// Suffix path (not-yet-ported techniques): render each element via its own
// operator<<.
template<class Container>
void print_section(std::ostream &outs, const char *tag, const Container &items, bool brace_each) {
    print_section_core(outs, tag, items, brace_each,
        [](std::ostream &o, auto const &item) { o << item; });
}

// Registry path (ported techniques): render each type-erased finding via
// Finding::print. Tag and brace flag come from the technique itself.
void print_section(std::ostream &outs, const Technique &tech, const FindingList &findings) {
    print_section_core(outs, tech.tag(), findings, tech.brace_each(),
        [](std::ostream &o, auto const &f) { f->print(o); });
}
} // namespace

std::ostream &operator<<(std::ostream &outs, Analyzer const &a) {
    // Registry prefix (cascade order), each line followed by endl -- byte-for-
    // byte identical to the hand-written lines it replaces because all framing
    // is shared via print_section_core. Safe to always trail with endl while
    // the suffix below is non-empty; the final port must reshape this.
    const auto &reg = Analyzer::registry();
    for (size_t i = 0; i < reg.size(); ++i) {
        print_section(outs, *reg[i], a.mFindings[i]); outs << std::endl;
    }
    print_section(outs, "HS", a.mHiddenSingles,    false); outs << std::endl;
    print_section(outs, "NP", a.mNakedPairs,       true);  outs << std::endl;
    print_section(outs, "LC", a.mLockedCandidates, true);  outs << std::endl;
    print_section(outs, "HP", a.mHiddenPairs,      true);  outs << std::endl;
    print_section(outs, "XW", a.mXWings,           true);  outs << std::endl;
    print_section(outs, "SC", a.mColorChains,      true);  outs << std::endl; // a.k.a. single's chains
    print_section(outs, "YW", a.mYWings,           true);  outs << std::endl;
    print_section(outs, "SF", a.mSwordfish,        true);  outs << std::endl;
    print_section(outs, "XY", a.mXYChains,         true);

    return outs;
}
