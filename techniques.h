// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

// Thin aggregator: pulls in every concrete Technique subclass so that
// Analyzer::registry() (in analyzer.cpp) can construct each one directly.
// Listed in cascade order; adding a technique is one new file plus one line
// here and one in registry() (issue #7).

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
