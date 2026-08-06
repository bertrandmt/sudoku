// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer-finnedswordfish.h"
#include "analyzer-util.h"
#include "board.h"
#include "row.h"
#include "column.h"
#include "nonet.h"
#include "cell.h"
#include "coord.h"
#include "verbose.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>
#include <type_traits>
#include <vector>

using analyzer_util::candidates;
using analyzer_util::line_of;

namespace {

// Find a finned Swordfish anchored on `cell` for `value`, with `cset` the first
// base line; `csets` are all lines parallel to `cset`, searched for the second and
// third base lines. find() stops at the first hit, so out holds at most one entry
// (asserted below).
//
// Where the plain fish *derives* its cover -- Swordfish rejects unless the union
// of cross lines is exactly three -- a finned fish has to *choose* three cover
// lines out of the union and bucket what is left over as fins. That choice is the
// whole difference in the search, and it is why a position like this one is
// invisible to find_swordfish: the union is four or more cross lines, so its
// equality test never matches.
template<class CandidateSet, class EliminationSet>
bool find_finned_swordfish(const Board &board, const Cell &cell, const Value &value,
                           const CandidateSet &cset, const std::vector<CandidateSet> &csets,
                           bool by_row, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));
    assert(cset.contains(cell));

    // A line holding a single candidate for the value is a hidden single, a
    // strictly cheaper deduction that fires much earlier in the cascade. The fish
    // family starts at two per base line.
    //
    // No test pins this floor, and none can through the solver: "exactly one
    // candidate for v in this line" *is* the hidden-single condition, so if it held
    // anywhere, HS would have fired and this technique would never have run. The
    // floor is unreachable-by-construction redundancy rather than a live gate, which
    // is why relaxing it to `< 1` leaves both suites green. A whitebox case could
    // reach it through the seam below, on a position the cascade cannot produce; that
    // is deliberately not done.
    //
    // Note what is deliberately *absent*: the plain Swordfish's upper bound of
    // three candidates per base line (analyzer-swordfish.cpp:50). That bound is
    // not an independent rule, it is the confinement requirement in disguise --
    // four candidates in a base line cannot fit three cover lines -- and
    // confinement is exactly what a fin relaxes. Here a fourth candidate is a
    // candidate fin, so bounding the count would reject the positions this
    // technique exists to find. Soundness does not depend on the bound: the
    // either/or needs only "every fin false => each base line confined to the
    // cover => a true Swordfish", which holds however many candidates a base line
    // started with.
    auto base1 = candidates(cset, value);
    if (base1.size() < 2) return false;

    // If cell is not the first candidate, we've already considered this cset
    if (cell != base1[0]) return false;

    for (auto const &cset2 : csets) {
        // Only consider subsequent csets
        if (!(cset < cset2)) continue;

        auto base2 = candidates(cset2, value);
        if (base2.size() < 2) continue;

        for (auto const &cset3 : csets) {
            // Only consider subsequent csets
            if (!(cset2 < cset3)) continue;

            auto base3 = candidates(cset3, value);
            if (base3.size() < 2) continue;

            // The distinct cross lines the three base lines touch, in
            // first-encounter order. Order matters -- it decides which cover
            // triple is tried first, and so which finding a first-hit search
            // records -- so this is a vector built by walking the candidates, not
            // a set of pointers whose order would follow addresses.
            std::vector<const EliminationSet *> crosses;
            for (auto const *side : { &base1, &base2, &base3 })
                for (auto const &c : *side) {
                    const EliminationSet *e = &line_of<EliminationSet>(board, c);
                    if (std::find(crosses.begin(), crosses.end(), e) == crosses.end())
                        crosses.push_back(e);
                }
            // Three base lines confined to three cross lines is a plain
            // Swordfish, and Swordfish runs ahead of this technique in the
            // cascade. A fin needs a fourth cross line to live on.
            if (crosses.size() < 4) continue;

            for (size_t i = 0; i < crosses.size(); i++) {
                for (size_t j = i + 1; j < crosses.size(); j++) {
                    for (size_t k = j + 1; k < crosses.size(); k++) {
                        const EliminationSet &e1 = *crosses[i];
                        const EliminationSet &e2 = *crosses[j];
                        const EliminationSet &e3 = *crosses[k];

                        // Split one base line's candidates against the chosen
                        // cover -- on it, or a fin outside it -- and report
                        // whether the line has anything on the cover at all.
                        std::vector<Coord> fins;
                        auto split = [&](const std::vector<Cell> &side) {
                            bool covered = false;
                            for (auto const &c : side) {
                                if (e1.contains(c) || e2.contains(c) || e3.contains(c)) { covered = true; continue; }
                                fins.push_back(c.coord());
                            }
                            return covered;
                        };
                        const bool base1_covered = split(base1);
                        const bool base2_covered = split(base2);
                        const bool base3_covered = split(base3);
                        // A base line with nothing on the cover is not a fish: the
                        // "every fin is false" branch would leave that line with
                        // no candidate at all, which is a contradiction -- a
                        // different, and stronger, deduction than the one this
                        // technique makes.
                        if (!base1_covered || !base2_covered || !base3_covered) continue;
                        // Not a gate but an invariant: `crosses` holds only lines
                        // some base candidate lies on, and there are more than
                        // three of them, so whichever three are chosen as cover, a
                        // candidate is left outside them. The finless case -- a
                        // plain Swordfish -- was already turned away by the
                        // crosses < 4 early-out above.
                        assert(!fins.empty());

                        // Every fin in one nonet is what licenses the elimination,
                        // and the gate is exact rather than merely safe. An
                        // eliminable cell lies on a cover line outside the base
                        // lines, so it shares no line with a fin: a fin's cross
                        // line is not a cover line (that is what made it a fin),
                        // and all three base lines are excluded. It can therefore
                        // see a fin only by sharing a nonet with it, and see
                        // *every* fin only if all the fins share that one nonet.
                        // Note the claim is about *eliminable* cells -- fins in a
                        // common base line are seen together by everything else on
                        // that line, so "no cell sees them all" would simply be
                        // false.
                        const Nonet &fin_nonet = board.nonet(fins.front());
                        bool one_nonet = true;
                        for (auto const &f : fins)
                            if (&board.nonet(f) != &fin_nonet) { one_nonet = false; break; }
                        if (!one_nonet) continue;

                        // There is something to eliminate iff a cell of the fin's
                        // nonet holds the value on a cover line, outside the base
                        // lines. Those are exactly the cells that see every fin
                        // *and* would be eliminated by the fish, so they are safe
                        // under either branch of the either/or. All three base
                        // lines have to be excluded here, not two: a cell of the
                        // third base line is part of the pattern, and striking it
                        // would be unsound.
                        bool has_eliminations = false;
                        for (auto const &c : fin_nonet) {
                            if (!c.isNote() || !c.check(value)) continue;
                            if (!e1.contains(c) && !e2.contains(c) && !e3.contains(c)) continue;
                            if (cset.contains(c) || cset2.contains(c) || cset3.contains(c)) continue;
                            has_eliminations = true;
                            break;
                        }
                        if (!has_eliminations) continue;

                        auto finding = std::make_shared<FinnedSwordfishFinding>(
                            value,
                            std::array<Coord, 3>{ base1[0].coord(), base2[0].coord(), base3[0].coord() },
                            std::move(fins),
                            by_row);
                        assert(out.empty());
                        if (sVerbose) { std::cout << "  [fFS] "; finding->print(std::cout); std::cout << std::endl; }
                        out.push_back(finding);
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// Apply one recorded finned Swordfish: clear `value` from the cells of the fin's
// nonet that lie on a cover line and outside the three base lines.
template<class EliminationSet>
bool act_on_finned_swordfish(Board &board, const FinnedSwordfishFinding &entry) {
    // The base lines and the elimination lines are always opposite kinds, so
    // derive one from the other rather than letting a caller pass a mismatched
    // pair (e.g. <Row, Row>) that would compile and silently misbehave.
    using CandidateSet = std::conditional_t<std::is_same_v<EliminationSet, Column>, Row, Column>;

    const CandidateSet &b1 = line_of<CandidateSet>(board, entry.anchors[0]);
    const CandidateSet &b2 = line_of<CandidateSet>(board, entry.anchors[1]);
    const CandidateSet &b3 = line_of<CandidateSet>(board, entry.anchors[2]);
    // find() proved this non-empty; front() below is undefined without it, and the
    // proof is on the far side of the find/apply boundary this finding crossed. So
    // restate it here, for the same reason the cover assert below exists: a
    // recorded finding's invariants are worth re-stating where they get used.
    assert(!entry.fins.empty());
    const Nonet &fin_nonet = board.nonet(entry.fins.front());

    // Recover the cover: the base candidates that are not fins all lie on it, and
    // there are exactly three such lines. Note what is *not* done here -- sweeping
    // every cross line the base lines touch, the way act_on_swordfish legitimately
    // does, would pull in the line the fin sits on. The recorded fin set is what
    // makes the difference recoverable, and the assert states the invariant that
    // makes this a recovery rather than a guess: each of the three chosen cover
    // lines was picked out of `crosses`, so it carries at least one base candidate,
    // and that candidate is on the cover and therefore not a fin -- so every cover
    // line comes back, and nothing else can.
    std::vector<const EliminationSet *> cover;
    for (const auto *line : { &b1, &b2, &b3 })
        for (auto const &c : candidates(*line, entry.value)) {
            if (std::find(entry.fins.begin(), entry.fins.end(), c.coord()) != entry.fins.end()) continue;
            const EliminationSet *e = &line_of<EliminationSet>(board, c);
            if (std::find(cover.begin(), cover.end(), e) == cover.end()) cover.push_back(e);
        }
    assert(cover.size() == 3);

    // `unit` is fully determined by EliminationSet -- derive it, don't thread it
    // through as a second source of truth a caller could get wrong. The tag names
    // the line the candidate is eliminated *from*; the fin's nonet is what narrows
    // which of that line's cells qualify.
    constexpr Unit unit = std::is_same_v<EliminationSet, Column> ? Unit::Column : Unit::Row;

    bool did_act = false;
    for (auto const &cell : fin_nonet) {
        if (!cell.isNote()) continue;
        if (!cell.check(entry.value)) continue;
        if (!cover[0]->contains(cell) && !cover[1]->contains(cell) && !cover[2]->contains(cell)) continue;
        // All three base lines, for the same reason find()'s has-eliminations scan
        // excludes all three: a cell of a base line belongs to the pattern, and the
        // either/or says nothing about it.
        if (b1.contains(cell) || b2.contains(cell) || b3.contains(cell)) continue;

        std::cout << "[FS] " << cell.coord() << " x" << entry.value << " [" << tag(unit) << "]" << std::endl;
        board.clear_note_at(cell.coord(), entry.value);
        did_act = true;
    }

    return did_act;
}

} // namespace

bool FinnedSwordfishTechnique::find_finned_swordfish(const Board &board, const Cell &cell, const Value &value, FindingList &out) {
    assert(cell.isNote());
    assert(cell.check(value));

    // Try row-based (eliminations in columns); if not found, try column-based
    // (eliminations in rows). EliminationSet cannot be deduced from the argument
    // list -- the cover is chosen, not handed in -- so both are named.
    if (::find_finned_swordfish<Row, Column>(board, cell, value, board.row(cell), board.rows(), true, out))
        return true;
    return ::find_finned_swordfish<Column, Row>(board, cell, value, board.column(cell), board.columns(), false, out);
}

// https://www.sudokuwiki.org/Finned_Swordfish
// A finned Swordfish is a Swordfish whose base lines are allowed extra candidates
// outside the three cover lines, so long as all of those fins share one nonet. The
// value is then eliminated from the cover lines' cells inside that nonet, outside
// the base lines.
bool FinnedSwordfishTechnique::find(const Board &board, FindingList &out) const {
    assert(out.empty());

    for (auto const &cell: board.cells()) {
        // is this a note cell?
        if (!cell.isNote()) continue;

        // for each value in this cell...
        for (auto const &value : cell.notes().values()) {
            // let's see if we can anchor a finned Swordfish in this cell for this
            // value; stop at the first one found
            if (find_finned_swordfish(board, cell, value, out)) return true;
        }
    }

    return false;
}

bool FinnedSwordfishTechnique::apply(Board &board, FindingList &mine) const {
    if (mine.empty()) return false;
    assert(mine.size() == 1);

    auto const &fs = bucket_cast<FinnedSwordfishFinding>(*mine.front());

    // Row-based pattern eliminates from columns; column-based, from rows.
    bool did_act = fs.is_row_based
        ? act_on_finned_swordfish<Column>(board, fs)
        : act_on_finned_swordfish<Row>(board, fs);

    mine.clear();
    assert(did_act);
    return did_act;
}
