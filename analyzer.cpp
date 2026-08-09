// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "techniques.h"
#include "board.h"

#include <cassert>
#include <iterator>
#include <memory>
#include <string_view>

// The stateless techniques, built once and shared by every Analyzer. This list
// *is* the solver's cascade: analyze() and act() walk it in order. Registering a
// technique takes two edits in this function -- the push_back below AND the
// kCascade literal above it, which the assert cross-checks -- and several more
// elsewhere; techniques.h enumerates every site. Deliberately no count here: one
// unenforced copy of that number is enough, and it belongs next to the list.
const std::vector<std::unique_ptr<Technique>> &Analyzer::registry() {
    // Canonical cascade order. Order is a correctness property, not a style
    // convention -- the cheapest technique that fires must fire first -- so it is
    // spelled out here once and checked against the built registry below, rather
    // than left implicit in the sequence of push_backs.
    [[maybe_unused]] static constexpr const char *kCascade[] = {
        "NS", "HS", "NP", "LC", "HP", "XW", "SC", "YW", "SF", "FX", "FS", "XY",
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
        r.push_back(std::make_unique<FinnedXWingTechnique>());
        r.push_back(std::make_unique<FinnedSwordfishTechnique>());
        r.push_back(std::make_unique<XYChainTechnique>());
        assert(r.size() == std::size(kCascade));
        for (size_t i = 0; i < r.size(); ++i)
            assert(std::string_view(r[i]->name()) == kCascade[i]);
        return r;
    }();
    return reg;
}

void Analyzer::analyze() {
    // No note-filtering pass here: Board::set_value_at maintains the peer
    // invariant at every placement, so this is a pure query over a board that
    // already holds it (see #8).
    for (auto &b : mFindings) b.clear();

    // Walk the cascade in order, each technique finding into its own bucket, and
    // stop at the first one that finds anything: the cheapest firing technique is
    // the one the solver applies.
    const auto &reg = registry();
    // mFindings is indexed in lockstep with reg (bucket i belongs to reg[i]);
    // the two are sized together at construction. Assert it here rather than
    // trust it: this positional coupling is what stands in for one separately
    // named member per registered technique, so it gets an executable guard.
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
    // operator<< streams the analyzer last and appends nothing, so the separator
    // form (not a terminator) is what keeps the trailing byte off.
    const auto &reg = Analyzer::registry();
    assert(a.mFindings.size() == reg.size());  // lockstep index; see analyze()
    for (size_t i = 0; i < reg.size(); ++i) {
        if (i > 0) outs << std::endl;
        print_section(outs, *reg[i], a.mFindings[i]);
    }

    return outs;
}
