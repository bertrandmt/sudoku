// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

// Thin aggregator: pulls in every concrete Technique subclass so that
// Analyzer::registry() (in analyzer.cpp) can construct each one directly.
// Listed in cascade order.
//
// Adding a technique is TEN sites. Each is followed by what catches it if you
// forget:
//   1. analyzer-<name>.h and analyzer-<name>.cpp  -- two new files    [compile]
//   2. a `src =` entry in the Makefile (explicit list, no glob)          [link]
//   3. the #include here                                             [compile]
//   4. the kCascade literal in analyzer.cpp                    [registry assert]
//   5. the registry() push_back in analyzer.cpp                [registry assert]
//   6. the bucket count and label in the unit suite's
//      test_rebinding_ctor_carries_findings                       [unit suite]
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
//  10. docs/test-predicate-idiom.md: the class table, the
//      with-`test_`-versus-inline count under it, and a bullet placing
//      the technique in its class and saying why                  [nothing]
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
// Six of the ten fail loudly -- at compile time, at link time, on an assert the
// shipped binary carries, or in the suite CI runs on every PR -- and so do (7a)
// and (7b). None of those can be missed in a way that leaves a technique quietly
// never firing, or leaves documented output quietly describing a solver that no
// longer exists.
//
// The remaining four -- (7c), (8), (9), (10) -- are unguarded, and for one shared
// reason: each is coverage or prose *for* the new technique, and nothing can know
// that a technique deserves a test or a section it does not yet have. Those four
// are what this list is really for. (8), (9) and (10) were missing from it until
// Finned X-Wing was added and needed all three, which is the argument for writing
// down even the sites no machine will ever check.

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
#include "analyzer-xychain.h"
