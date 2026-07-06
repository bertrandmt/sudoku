// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

// Thin aggregator: pulls in every concrete Technique subclass so that
// Analyzer::registry() (in analyzer.cpp) can construct each one directly.
// One #include is added here per technique as it ports (cascade order).

#include "analyzer-nakedsingles.h"
#include "analyzer-hiddensingles.h"
#include "analyzer-nakedpairs.h"
#include "analyzer-lockedcandidates.h"
#include "analyzer-hiddenpairs.h"
