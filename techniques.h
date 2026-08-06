// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

// Thin aggregator: pulls in every concrete Technique subclass so that
// Analyzer::registry() (in analyzer.cpp) can construct each one directly.
// Listed in cascade order.
//
// Adding a technique is TWELVE sites. Each is followed by what catches it if you
// forget:
//   1. analyzer-<name>.h and analyzer-<name>.cpp  -- two new files    [compile]
//   2. a `src =` entry in the Makefile (explicit list, no glob)          [link]
//   3. the #include here                                             [compile]
//   4. the kCascade literal in analyzer.cpp                    [registry assert]
//   5. the registry() push_back in analyzer.cpp                [registry assert]
//   6. every technique count in the unit suite -- the bucket count and its
//      label in test_rebinding_ctor_carries_findings, plus any count in the
//      prose around it. The count is what fails; the prose is not [unit suite]
//   7. README.md, in three parts:
//      a. an entry in the numbered technique list ("denoted as
//         `[XY]`")                                            [tests/run.sh]
//      b. regenerated dump blocks -- every full dump prints one line
//         per registry entry, and each worked example registered in
//         run.sh is diffed against live solver output         [tests/run.sh]
//      c. a `## <Name>` heuristic section for the new technique   [nothing]
//   8. a tests/run.sh tier [3] check_tech line, proving the technique
//      applies at least one elimination somewhere. If no board already in
//      the suite routes through it, that also means a new P_/S_ fixture
//      and its name in tier [0]/[1]/[2]'s shared loop            [nothing]
//   9. a tests/run.sh tier [7] prec_check block, pinning the exact
//      candidates its first application removes                   [nothing]
//  10. docs/test-predicate-idiom.md: a bullet placing the technique in its
//      class and saying why, and **every technique count in the file**, not
//      an enumerated few. Enumerating is what failed here: the first pass
//      updated the class table and the partition line and left two counts
//      in the prose stale                                         [nothing]
//  11. a whitebox seam decision, and usually cases to go with it: whether the
//      per-anchor entry is promoted to a public static and whether the
//      Finding leaves its .cpp for the header. Conditional, like the fixture
//      in (8) -- locked candidates carries no whitebox cases at all and so
//      needs no seam -- so what is mandatory is *deciding*, and recording
//      that decision in (10), not writing cases either way        [nothing]
//  12. notes.txt, in two parts. Its per-board annotations lay the techniques out
//      in fixed 4-character columns indexed by cascade position, so a technique
//      inserted anywhere but the end silently misaligns every tag to its right
//      (Finned X-Wing shifted [XY] by one column, Finned Swordfish by another),
//      and a board the new technique fires on wants its own tag added. Reference
//      data, deliberately not asserted by tier [6], hence unguarded  [nothing]
// (4) and (5) cross-check each other: the assert compares the built registry's
// size and names against kCascade, so getting exactly one of them wrong aborts.
// (7a) and (7b) read what they expect out of the binary rather than hardcoding
// it, so they track this list by construction and cannot themselves go stale.
// (7c) is unguarded in one direction only: nothing forces a *new* technique to be
// given its own section, because checking that means mapping "XY" to
// "## XY-Chain" and the registry does not know that. A section that does exist is
// covered -- run.sh replays each registered example's fixture board and diffs the
// block, and its coverage check fails on a section whose dump block nobody
// registered in that table.
//
// README's list is the project's only tag-to-name mapping, and (7a) is what keeps
// it honest. Do not stand up a second copy elsewhere: nothing would check it, and
// it would go stale the first time this list grew.
//
// Six fail loudly -- at compile time, at link time, on an assert the shipped
// binary carries, or in the suite CI runs on every PR -- and so do (7a) and (7b).
// None of those can be missed in a way that leaves a technique quietly never
// firing, or leaves documented output quietly describing a solver that no longer
// exists. (6) is the awkward one: its count fails the unit suite, but the prose
// naming the techniques beside it does not.
//
// The remaining six -- (7c), (8), (9), (10), (11), (12) -- are unguarded. Five of
// them share one reason: each is coverage or prose *for* the new technique, and
// nothing can know that a technique deserves a test, a section, or a seam it does
// not yet have. (12) is unguarded for a different reason -- it is reference data
// about real puzzles, which the suite reads for boards and deliberately does not
// trust for annotations. Those six are what this list is really for; the loud six
// would announce themselves without it.
//
// A note on the number, because it is now large enough to be misread as a cost the
// technique registry (issue #7) introduced. It is not. The list grows because sites
// are being *recognised*, not created: notes.txt, tests/run.sh and
// docs/test-predicate-idiom.md all predate the first registry commit, and each was
// already work an author had to do and this list failed to name. Meanwhile the two
// sites the registry did create, (4) and (5), replaced five silent ones -- a member
// vector, a clear() in analyze(), a find-chain entry, an act-chain entry and a
// print_section line -- with two that cross-check each other and abort. Against the
// pre-registry status quo this is neutral at worst, and better on the loud sites.
//
// Two lessons are baked into the wording above, both learned by getting it wrong.
// (8) through (11) were missing entirely until Finned X-Wing needed all four, so
// a site absent from this list is not a site that does not exist. And (6) and
// (10) say "every count in the file" rather than naming places, because naming
// places is exactly what went stale: the same change that added (10) to this list
// updated two of the four counts in that file and missed the other two.

#include "analyzer-nakedsingles.h"
#include "analyzer-hiddensingles.h"
#include "analyzer-nakedpairs.h"
#include "analyzer-lockedcandidates.h"
#include "analyzer-hiddenpairs.h"
#include "analyzer-xwing.h"
#include "analyzer-colorchain.h"
#include "analyzer-ywing.h"
#include "analyzer-swordfish.h"
#include "analyzer-finnedxwing.h"
#include "analyzer-finnedswordfish.h"
#include "analyzer-xychain.h"
