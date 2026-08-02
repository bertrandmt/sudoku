// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

// Thin aggregator: pulls in every concrete Technique subclass so that
// Analyzer::registry() (in analyzer.cpp) can construct each one directly.
// Listed in cascade order.
//
// Adding a technique is SIX sites, not the "one new file plus one registration
// line" issue #7's write-up promised. Each is followed by what catches it if you
// forget:
//   1. analyzer-<name>.h and analyzer-<name>.cpp  -- two new files    [compile]
//   2. a `src =` entry in the Makefile (explicit list, no glob)          [link]
//   3. the #include here                                             [compile]
//   4. the kCascade literal in analyzer.cpp                    [registry assert]
//   5. the registry() push_back in analyzer.cpp                [registry assert]
//   6. the bucket count and label in the unit suite's
//      test_rebinding_ctor_carries_findings                       [unit suite]
// (4) and (5) cross-check each other: the assert compares the built registry's
// size and names against kCascade, so getting exactly one of them wrong aborts.
//
// So what #7 bought is not really the smaller number -- it is that the number is
// now self-enforcing. Of the eight sites it replaced, five were silent: forget
// the analyze() entry, the act() entry, the operator<< line, the clear(), or the
// rebinding-ctor initializer, and the technique just quietly never fired. That
// hazard is what the issue was filed about, and it is the part that is gone.

#include "analyzer-nakedsingles.h"
#include "analyzer-hiddensingles.h"
#include "analyzer-nakedpairs.h"
#include "analyzer-lockedcandidates.h"
#include "analyzer-hiddenpairs.h"
#include "analyzer-xwing.h"
#include "analyzer-colorchain.h"
#include "analyzer-ywing.h"
#include "analyzer-swordfish.h"
#include "analyzer-xychain.h"
