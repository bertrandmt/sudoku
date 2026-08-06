// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "coord.h"
#include "technique.h"
#include "verbose.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// The fish family -- X-Wing, Swordfish, and the finned variant of each -- is one
// rule at two sizes, and this is where that rule lives.
//
// A fish for one value is N *base lines* (all rows, or all columns) whose
// candidates for the value all fall on N *cover lines* of the opposite kind.
// Each cover line then has to spend its candidate for the value on a base line,
// so every other candidate for it on the cover can go. X-Wing is that at N=2,
// Swordfish at N=3.
//
// A *finned* fish relaxes the confinement: base candidates are allowed off the
// cover -- the fins -- provided every fin sits in a single nonet. If every fin
// is false the plain fish holds after all; if some fin is true its nonet already
// holds the value; so the cells safe under both branches are those that see
// every fin, which is the fin's nonet. Finned X-Wing is that at N=2, finned
// Swordfish at N=3.
//
// There are deliberately *two* workers here rather than one (#58 weighed the
// alternative): find_plain_fish and find_finned_fish, each parameterised by N.
// The plain fish is the fins-empty case of the finned one mathematically, but a
// single worker would have carried a mode flag whose justification is cascade
// bookkeeping rather than the rule -- finned X-Wing rejects a finless position
// only because plain X-Wing runs earlier in the cascade and has already fired on
// it -- plus a region switch in both the find and the act. Each technique still
// owns its registry entry, tag, Finding subtype, bucket, tier and file; what is
// shared is the scan body, not the technique.
//
// Two orderings in here are output contracts rather than implementation details,
// and each is called out where it is established:
//   - the order cover subsets are tried (for_each_cover), because every fish
//     stops at its first hit, so it decides which pattern gets recorded;
//   - the order cover lines are swept in act (cover_of), because it decides the
//     order the elimination lines print in.
//
// Wrapped in a named namespace so these collision-prone generic names do not
// land at global scope for every TU that includes this header.
namespace analyzer_fish {

// The cells of `set` that still hold `value` as a note.
template<class Set>
std::vector<Cell> candidates(const Set &set, const Value &value) {
    std::vector<Cell> candidates;

    for (auto const &cell : set) {
        if (!cell.isNote()) continue;
        if (!cell.check(value)) continue;

        candidates.push_back(cell);
    }

    return candidates;
}

// The Row or Column (selected by the Line type) that `x` (a Cell or Coord)
// lies on. Board exposes both row()/column() overloads for Cell and Coord, so
// one helper serves find and act, for cells and coords, with the row/column
// choice made once here instead of at every call site.
template<class Line, class CellOrCoord>
const Line &line_of(const Board &board, const CellOrCoord &x) {
    if constexpr (std::is_same_v<Line, Column>) return board.column(x);
    else                                        return board.row(x);
}

// The N base lines of a fish, each paired with its candidate cells for the value
// under search. `lines[i]` and `candidates[i]` are parallel, so a fish's own
// vocabulary -- "the first candidate of the second base line" -- indexes
// straight through. A search fills these one line at a time, so a prefix of the
// first `count` entries is meaningful before all N are chosen.
template<class CandidateSet, size_t N>
struct Bases {
    std::array<const CandidateSet *, N> lines{};
    std::array<std::vector<Cell>, N>    candidates{};

    // Does `cell` lie on one of the base lines? The base lines hold the pattern,
    // so their own candidates are part of the fish and are never eliminated.
    // This is the one gate the family cannot get wrong and stay sound, and it is
    // the reason it is a fold over a container here rather than the chain of
    // `cset.contains(c) || cset2.contains(c) || ...` terms each fish used to
    // spell out at its own arity: a fold cannot be written with a term missing.
    bool contains(const Cell &cell) const {
        for (const auto *line : lines)
            if (line->contains(cell)) return true;
        return false;
    }
};

// The first candidate cell of each base line. This is how a recorded finding
// gets its base lines back: apply() calls line_of on each anchor, so an anchor
// identifies a line and nothing more -- for a finned fish it may itself be a
// fin. Built through an index_sequence rather than a loop because Coord has no
// default constructor, so a std::array of them cannot be filled after the fact.
template<class CandidateSet, size_t N, size_t... I>
std::array<Coord, N> anchors_of(const Bases<CandidateSet, N> &bases, std::index_sequence<I...>) {
    return { bases.candidates[I][0].coord()... };
}

template<class CandidateSet, size_t N>
std::array<Coord, N> anchors_of(const Bases<CandidateSet, N> &bases) {
    return anchors_of(bases, std::make_index_sequence<N>{});
}

// Does `cell` lie on any of `lines`? Takes any range of line pointers, so the
// same fold serves the cover-as-vector a search builds and the cover-as-array a
// recorded finding recovers.
template<class Lines>
bool on_any(const Lines &lines, const Cell &cell) {
    for (const auto *line : lines)
        if (line->contains(cell)) return true;
    return false;
}

// The distinct cross lines that the first `count` base lines' candidates touch,
// in first-encounter order: base line by base line, and within each line in the
// order its candidates are walked.
//
// That order is load-bearing for a finned fish, where it decides which cover
// subset for_each_cover tries first and so which pattern a first-hit search
// records. It is deliberately *not* a set of pointers: a std::set here would
// order by address, making the recorded pattern a function of the board's memory
// layout.
//
// `count` is what lets a search prune a partial pattern: a plain fish rejects a
// prefix whose cross lines already exceed its cover budget without building the
// rest of the set.
template<class EliminationSet, class CandidateSet, size_t N>
std::vector<const EliminationSet *> crosses_of(const Board &board,
        const Bases<CandidateSet, N> &bases, size_t count) {
    assert(count <= N);

    std::vector<const EliminationSet *> crosses;
    for (size_t i = 0; i < count; i++)
        for (auto const &c : bases.candidates[i]) {
            const EliminationSet *e = &line_of<EliminationSet>(board, c);
            if (std::find(crosses.begin(), crosses.end(), e) == crosses.end())
                crosses.push_back(e);
        }

    return crosses;
}

// Recover a recorded fish's cover: the cross lines its base candidates lie on,
// skipping the ones only a fin reaches. Every cover line comes back, and nothing
// else can -- each was chosen out of the crosses, so it carries at least one base
// candidate, and that candidate is on the cover and therefore not a fin -- which
// is what the size assert states, and what makes this a recovery rather than a
// guess. For a plain fish `fins` is empty and the cover is simply every cross
// line the base candidates touch.
//
// That assert is a bound, not a restatement: the std::copy below writes into a
// fixed-size array, so a cover that came back larger than N would overflow it. It
// cannot -- find() required exactly N cross lines on this same board, and
// SolverState re-analyzes after every mutation, so a finding is always applied to
// the board its find ran on -- but the assert is the only thing standing between
// that reasoning and a stack write, and asserts ship here (no -DNDEBUG). Left as
// std::copy rather than copy_n deliberately: bounding the write would trade an
// abort for a silent truncation, which is worse to debug.
//
// Note what is *not* done: sweeping every cross line for a *finned* fish, without
// subtracting the fins, would drag in the line the fin sits on, which the fish
// says nothing about.
//
// Sorted, and that is an output contract: act sweeps the cover in this order, so
// it is the order a fish's elimination lines print in. The line types order by
// index, which is the order the plain fish printed in before they shared this
// code -- X-Wing swept the cover line through its anchor before the one through
// its diagonal, and Swordfish swept a std::set of line pointers whose addresses
// ascend with index because Board holds its lines in a vector. Neither is a
// property to leave resting on memory layout, hence an explicit sort by index.
template<class EliminationSet, class CandidateSet, size_t N>
std::array<const EliminationSet *, N> cover_of(const Board &board,
        const Bases<CandidateSet, N> &bases, const std::vector<Coord> &fins) {
    std::vector<const EliminationSet *> cover;
    for (size_t i = 0; i < N; i++)
        for (auto const &c : bases.candidates[i]) {
            if (std::find(fins.begin(), fins.end(), c.coord()) != fins.end()) continue;
            const EliminationSet *e = &line_of<EliminationSet>(board, c);
            if (std::find(cover.begin(), cover.end(), e) == cover.end())
                cover.push_back(e);
        }
    assert(cover.size() == N);

    std::sort(cover.begin(), cover.end(),
              [](const EliminationSet *a, const EliminationSet *b) { return *a < *b; });

    std::array<const EliminationSet *, N> sorted{};
    std::copy(cover.begin(), cover.end(), sorted.begin());
    return sorted;
}

// Would this fish eliminate `cell`? It holds the value, it lies on a cover line,
// and it lies outside every base line. Both halves of every fish run through this
// one predicate: find asks whether a region holds such a cell at all, act clears
// the value from each one it finds, and the plain and finned fish differ only in
// the region they ask about -- the cover lines themselves, versus the fin's nonet.
template<class Cover, class CandidateSet, size_t N>
bool eliminable(const Cell &cell, const Value &value, const Cover &cover,
                const Bases<CandidateSet, N> &bases) {
    if (!cell.isNote()) return false;
    if (!cell.check(value)) return false;
    if (!on_any(cover, cell)) return false;
    return !bases.contains(cell);
}

// Does `region` hold anything this fish would eliminate? A fish that eliminates
// nothing is not recorded: it is a true statement about the board that moves the
// solver nowhere.
template<class Region, class Cover, class CandidateSet, size_t N>
bool any_eliminable(const Region &region, const Value &value, const Cover &cover,
                    const Bases<CandidateSet, N> &bases) {
    for (auto const &cell : region)
        if (eliminable(cell, value, cover, bases)) return true;
    return false;
}

// Grow `bases` -- already filled through `filled` lines -- by every ascending
// choice of the lines still missing, and call `body(bases)` on each complete set
// of N. `accept(bases, i)` gates line `i` as it is added, which is where a family
// puts both its per-line requirement and any prune of a partial pattern; a
// rejected line is not recursed through.
//
// Ascending order only, so a set of base lines is considered once: {r1,r4} is
// reached and {r4,r1} is not. Stops at the first `body` returning true, which is
// what makes every fish a first-hit search.
template<class CandidateSet, size_t N, class Accept, class Body>
bool extend_bases(const std::vector<CandidateSet> &csets, const Value &value,
                  Bases<CandidateSet, N> &bases, size_t filled,
                  const Accept &accept, const Body &body) {
    assert(filled > 0);  // the anchor's own line fills bases.lines[0]
    if (filled == N) return body(bases);

    for (auto const &next : csets) {
        if (!(*bases.lines[filled - 1] < next)) continue;

        bases.lines[filled] = &next;
        bases.candidates[filled] = candidates(next, value);
        if (!accept(bases, filled)) continue;

        if (extend_bases(csets, value, bases, filled + 1, accept, body)) return true;
    }

    return false;
}

// Every N-line subset of `crosses`, lexicographic by index, handed to `body` in
// `cover`. At two base lines that is the (i, j) pair walk a finned X-Wing spells
// out and at three the (i, j, k) triple walk a finned Swordfish spells out -- the
// same subsets in the same order, which is what keeps a first-hit search
// recording the pattern it recorded before this was shared. Stops at the first
// `body` returning true.
template<class EliminationSet, size_t N, class Body>
bool for_each_cover(const std::vector<const EliminationSet *> &crosses,
                    std::array<const EliminationSet *, N> &cover, size_t chosen, size_t from,
                    const Body &body) {
    if (chosen == N) return body(cover);

    // Stop once too few lines remain to finish the subset.
    for (size_t i = from; i + (N - chosen) <= crosses.size(); i++) {
        cover[chosen] = crosses[i];
        if (for_each_cover(crosses, cover, chosen + 1, i + 1, body)) return true;
    }

    return false;
}

// Find a plain fish of N base lines, anchored on `cell` for `value`, with `cset`
// the first base line and `csets` all the lines parallel to it. `record` turns the
// pattern into the technique's own Finding; `name` is its output tag. find() stops
// at the first hit, so `out` holds at most one entry (asserted below).
//
// This is X-Wing at N=2 and Swordfish at N=3.
template<size_t N, class CandidateSet, class EliminationSet, class Record>
bool find_plain_fish(const Board &board, const Cell &cell, const Value &value,
                     const CandidateSet &cset, const std::vector<CandidateSet> &csets,
                     const char *name, const Record &record, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));

    // Confinement, checked as the pattern grows: N base lines whose candidates
    // fall on N cross lines. A prefix already touching more than N cross lines
    // cannot be completed into a fish, so it is rejected before the rest of the
    // set is built.
    //
    // This one budget subsumes the per-line candidate bounds the two plain fish
    // used to spell out separately -- X-Wing's "exactly two per base line",
    // Swordfish's "two or three" -- because a line's candidates for one value lie
    // one per cross line, so "at most N candidates in the line" and "at most N
    // cross lines touched" are the same bound. The floor is a separate matter and
    // is not a bound of the fish at all: a line holding a *single* candidate for
    // the value is a hidden single, a strictly cheaper deduction that fires much
    // earlier in the cascade, so the family starts at two per base line.
    auto accept = [&](const Bases<CandidateSet, N> &b, size_t filled) {
        if (b.candidates[filled].size() < 2) return false;
        return crosses_of<EliminationSet>(board, b, filled + 1).size() <= N;
    };

    Bases<CandidateSet, N> bases;
    bases.lines[0] = &cset;
    bases.candidates[0] = candidates(cset, value);
    if (!accept(bases, 0)) return false;

    // If cell is not the first candidate, we've already considered this cset and
    // found it unsuitable.
    if (cell != bases.candidates[0][0]) return false;

    return extend_bases(csets, value, bases, 1, accept, [&](const Bases<CandidateSet, N> &b) {
        // Exactly N cover lines. `accept` held the union at N or below, so what is
        // left to reject is a union that came out *short*: three base lines
        // sharing two cross lines, say. Vacuous at N=2 -- a first base line of
        // exactly two candidates touches two cross lines and the prune keeps it
        // there -- and a live gate at N=3.
        //
        // Deliberately recomputed rather than carried out of `accept`, which
        // evaluated the same walk one call earlier to test `<= N`. Handing the
        // cover back would mean `accept` returning a value instead of a verdict,
        // complicating extend_bases' contract for both families to save a walk over
        // at most nine cells, and it does not show up in a corpus benchmark.
        auto cover = crosses_of<EliminationSet>(board, b, N);
        if (cover.size() != N) return false;

        // There is something to eliminate iff some cover line holds a candidate
        // for `value` outside the base lines. Counting ">N candidates in the line"
        // is not equivalent at three base lines: a cover line hit by only two of
        // the three can have exactly three candidates, one of which lies outside
        // the pattern and is eliminable. (At two base lines it *is* equivalent,
        // every cover line carrying exactly one candidate per base line, which is
        // why X-Wing could ask the question by count.)
        bool has_eliminations = false;
        for (const auto *line : cover)
            if (any_eliminable(*line, value, cover, b)) { has_eliminations = true; break; }
        if (!has_eliminations) return false;

        auto finding = record(b);
        assert(out.empty());
        if (sVerbose) { std::cout << "  [f" << name << "] "; finding->print(std::cout); std::cout << std::endl; }
        out.push_back(finding);
        return true;
    });
}

// Find a finned fish of N base lines, anchored on `cell` for `value`, with `cset`
// the first base line and `csets` all the lines parallel to it. `record` turns the
// pattern and its fins into the technique's own Finding; `name` is its output tag.
// find() stops at the first hit, so `out` holds at most one entry (asserted
// below).
//
// This is finned X-Wing at N=2 and finned Swordfish at N=3.
//
// Where a plain fish *derives* its cover -- rejecting unless the cross lines union
// to exactly N -- a finned fish has to *choose* N cover lines out of the union and
// bucket what is left over as fins. That choice is the whole difference in the
// search, and it is why a finned position is invisible to find_plain_fish: the
// union is N+1 cross lines or more, so its size test never matches.
template<size_t N, class CandidateSet, class EliminationSet, class Record>
bool find_finned_fish(const Board &board, const Cell &cell, const Value &value,
                      const CandidateSet &cset, const std::vector<CandidateSet> &csets,
                      const char *name, const Record &record, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));

    // A line holding a single candidate for the value is a hidden single, a
    // strictly cheaper deduction that fires much earlier in the cascade. The fish
    // family starts at two per base line.
    //
    // No test pins this floor, and none can through the solver: "exactly one
    // candidate for v in this line" *is* the hidden-single condition, so if it
    // held anywhere, HS would have fired and no fish would have run. The floor is
    // unreachable-by-construction redundancy rather than a live gate, which is why
    // relaxing it to `< 1` leaves both suites green. A whitebox case could reach
    // it through a technique's seam, on a position the cascade cannot produce;
    // that is deliberately not done.
    //
    // Note what is deliberately *absent*: find_plain_fish's upper bound of N
    // candidates per base line. That bound is not an independent rule, it is the
    // confinement requirement in disguise -- N+1 candidates in a base line cannot
    // fit N cover lines -- and confinement is exactly what a fin relaxes. Here an
    // extra candidate is a candidate fin, so bounding the count would reject the
    // positions this search exists to find. Soundness does not depend on the
    // bound: the either/or needs only "every fin false => each base line confined
    // to the cover => a true fish", which holds however many candidates a base
    // line started with.
    auto accept = [](const Bases<CandidateSet, N> &b, size_t filled) {
        return b.candidates[filled].size() >= 2;
    };

    Bases<CandidateSet, N> bases;
    bases.lines[0] = &cset;
    bases.candidates[0] = candidates(cset, value);
    if (!accept(bases, 0)) return false;

    // If cell is not the first candidate, we've already considered this cset.
    if (cell != bases.candidates[0][0]) return false;

    return extend_bases(csets, value, bases, 1, accept, [&](const Bases<CandidateSet, N> &b) {
        auto crosses = crosses_of<EliminationSet>(board, b, N);
        // N base lines confined to N cross lines is a plain fish, and the plain
        // fish runs ahead of its finned variant in the cascade. A fin needs an
        // (N+1)th cross line to live on.
        if (crosses.size() < N + 1) return false;

        std::array<const EliminationSet *, N> cover{};
        return for_each_cover(crosses, cover, 0, 0,
                              [&](const std::array<const EliminationSet *, N> &chosen) {
            // Split each base line's candidates against the chosen cover -- on it,
            // or a fin outside it -- and note whether the line has anything on the
            // cover at all. Every line is split before any verdict is reached, so
            // the fins come out in base-line-major order however the verdict goes.
            std::vector<Coord> fins;
            bool all_covered = true;
            for (size_t i = 0; i < N; i++) {
                bool covered = false;
                for (auto const &cand : b.candidates[i]) {
                    if (on_any(chosen, cand)) { covered = true; continue; }
                    fins.push_back(cand.coord());
                }
                if (!covered) all_covered = false;
            }
            // A base line with nothing on the cover is not a fish: the "every fin
            // is false" branch would leave that line with no candidate at all,
            // which is a contradiction -- a different, and stronger, deduction
            // than the one a fish makes.
            if (!all_covered) return false;
            // Not a gate but an invariant: `crosses` holds only lines some base
            // candidate lies on, and there are more than N of them, so whichever N
            // are chosen as cover, a candidate is left outside them. The finless
            // case -- a plain fish -- was already turned away by the crosses < N+1
            // early-out above.
            assert(!fins.empty());

            // Every fin in one nonet is what licenses the elimination, and the
            // gate is exact rather than merely safe. An eliminable cell lies on a
            // cover line outside the base lines, so it shares no line with a fin:
            // a fin's cross line is not a cover line (that is what made it a fin),
            // and every base line is excluded. It can therefore see a fin only by
            // sharing a nonet with it, and see *every* fin only if all the fins
            // share that one nonet. Note the claim is about *eliminable* cells --
            // fins in a common base line are seen together by everything else on
            // that line, so "no cell sees them all" would simply be false. The
            // argument does not depend on N, and was re-derived at three base
            // lines rather than adapted from two.
            const Nonet &fin_nonet = board.nonet(fins.front());
            bool one_nonet = true;
            for (auto const &f : fins)
                if (&board.nonet(f) != &fin_nonet) { one_nonet = false; break; }
            if (!one_nonet) return false;

            // There is something to eliminate iff a cell of the fin's nonet holds
            // the value on a cover line, outside the base lines. Those are exactly
            // the cells that see every fin *and* would be eliminated by the fish,
            // so they are safe under either branch of the either/or.
            if (!any_eliminable(fin_nonet, value, chosen, b)) return false;

            auto finding = record(b, std::move(fins));
            assert(out.empty());
            if (sVerbose) { std::cout << "  [f" << name << "] "; finding->print(std::cout); std::cout << std::endl; }
            out.push_back(finding);
            return true;
        });
    });
}

// The base lines of a recorded fish, recovered from its anchors. The base lines
// and the cover are always opposite kinds, so CandidateSet is derived from
// EliminationSet rather than left for a caller to pass -- a mismatched pair (say
// <Row, Row>) would compile and silently misbehave.
template<class EliminationSet, size_t N>
auto bases_of(const Board &board, const std::array<Coord, N> &anchors, const Value &value) {
    using CandidateSet = std::conditional_t<std::is_same_v<EliminationSet, Column>, Row, Column>;

    Bases<CandidateSet, N> bases;
    for (size_t i = 0; i < N; i++) {
        bases.lines[i] = &line_of<CandidateSet>(board, anchors[i]);
        bases.candidates[i] = candidates(*bases.lines[i], value);
    }
    return bases;
}

// The unit an elimination is reported against is fully determined by
// EliminationSet -- derive it, don't thread it through as a second source of
// truth a caller could get wrong.
template<class EliminationSet>
constexpr Unit unit_of() {
    return std::is_same_v<EliminationSet, Column> ? Unit::Column : Unit::Row;
}

// Apply one recorded plain fish: clear `value` from every cell of the cover that
// is not on a base line. `name` is the technique's output tag.
template<size_t N, class EliminationSet>
bool act_on_plain_fish(Board &board, const Value &value, const std::array<Coord, N> &anchors,
                       const char *name) {
    const auto bases = bases_of<EliminationSet>(board, anchors, value);
    const auto cover = cover_of<EliminationSet>(board, bases, {});

    // This sweeps each cover line *live* while clearing notes in it, where the
    // pre-#58 X-Wing swept a vector snapshot of the line's candidates taken before
    // any elimination. Equivalent, and it rests on two facts from elsewhere:
    // Board::clear_note_at strikes one candidate in one cell and cascades nothing,
    // and Cell::operator== compares coordinates only, so contains() is a coord
    // test unaffected by notes changing under it. A future elimination that
    // cascades would break this loop and not obviously.
    bool did_act = false;
    for (const auto *line : cover)
        for (auto const &cell : *line) {
            if (!eliminable(cell, value, cover, bases)) continue;

            std::cout << "[" << name << "] " << cell.coord() << " x" << value
                      << " [" << tag(unit_of<EliminationSet>()) << "]" << std::endl;
            board.clear_note_at(cell.coord(), value);
            did_act = true;
        }

    return did_act;
}

// Apply one recorded finned fish: clear `value` from the cells of the fin's nonet
// that lie on a cover line and outside the base lines. `name` is the technique's
// output tag.
template<size_t N, class EliminationSet>
bool act_on_finned_fish(Board &board, const Value &value, const std::array<Coord, N> &anchors,
                        const std::vector<Coord> &fins, const char *name) {
    const auto bases = bases_of<EliminationSet>(board, anchors, value);
    const auto cover = cover_of<EliminationSet>(board, bases, fins);
    // find() proved this non-empty; front() below is undefined without it, and the
    // proof is on the far side of the find/apply boundary this finding crossed. So
    // restate it here, for the same reason cover_of asserts its own size: a
    // recorded finding's invariants are worth re-stating where they get used.
    assert(!fins.empty());
    const Nonet &fin_nonet = board.nonet(fins.front());

    // The tag names the line the candidate is eliminated *from*; the fin's nonet
    // is what narrows which of that line's cells qualify.
    bool did_act = false;
    for (auto const &cell : fin_nonet) {
        if (!eliminable(cell, value, cover, bases)) continue;

        std::cout << "[" << name << "] " << cell.coord() << " x" << value
                  << " [" << tag(unit_of<EliminationSet>()) << "]" << std::endl;
        board.clear_note_at(cell.coord(), value);
        did_act = true;
    }

    return did_act;
}

} // namespace analyzer_fish
