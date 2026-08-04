// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
//
// Whitebox unit tests for the interactive tab-completion core, complete_move().
//
// The black-box suite (tests/run.sh) drives the compiled REPL through a pipe,
// which never exercises libedit's completion callbacks at all -- those only fire
// on an interactive terminal. complete_move() was deliberately factored out as
// a pure function (board state in, candidate list out) precisely so the staged
// completion logic can be tested here without a pty: only the thin readline glue
// in sudoku-solver.cpp stays untested.
//
// Framework-free on purpose, matching the project's no-dependency testing style.
// Each check() records a line; a nonzero exit code means a failure.

#include "completion.h"
#include "solver.h"

#include <iostream>
#include <string>
#include <vector>

// The analyzer (run when a Solver is constructed) reads this application-global
// to decide whether to narrate; it is normally defined in the REPL main, which
// this test binary does not link, so we supply it here and keep it quiet.
bool sVerbose = false;

namespace {

int failures = 0;
void check(bool cond, const std::string &msg) {
    if (cond) { std::cout << "  ok    " << msg << "\n"; }
    else      { std::cout << "  FAIL  " << msg << "\n"; ++failures; }
}

// A Solver over a board described in the REPL's "dot" form: a leading '.'
// marker followed by 81 cells (digit or '.'). Mirrors what 'n.<board>' builds.
Solver make_solver(const std::string &cells81) {
    return Solver("." + cells81);
}

bool eq(const std::vector<std::string> &got, const std::vector<std::string> &want) {
    return got == want;
}

// A fully empty board: every cell unset, every candidate present.
const std::string kEmpty(81, '.');

// The "medium" fixture from tests/run.sh. Row 1 is "3........": cell (1,1) is
// the given clue 3, the other eight cells of the row are unset.
const std::string kMed =
    "3........97..1....6..583...2.....9..5..621..3..8.....5...435..2....9..56........1";

// The "color" fixture from tests/run.sh, chosen because rows 3, 5 and 7 are
// *fully* given ("517283964", "145836729", "451378296"). kEmpty and kMed both
// have an unset cell in every row, so neither can observe stage 0 rejecting a
// row -- deleting that filter entirely leaves them green. This board is what
// pins it. Loading a board only analyzes, never places, so the set cells are
// exactly the non-'.' characters here.
const std::string kColor =
    "289...375364.9.812517283964893.2.6.1145836729726....83451378296.72.1..38.38..21.7";

// ---------------------------------------------------------------------------

void test_stage0_rows() {
    std::cout << "[complete] stage 0: rows that still hold an unset cell\n";
    Solver empty = make_solver(kEmpty);
    check(eq(complete_move(empty, ""), {"1","2","3","4","5","6","7","8","9"}),
          "empty board offers all nine rows");

    // Even with clues scattered through it, kMed has at least one unset cell in
    // every row, so all nine rows are still offered.
    Solver med = make_solver(kMed);
    check(eq(complete_move(med, ""), {"1","2","3","4","5","6","7","8","9"}),
          "kMed offers all nine rows (each row has an unset cell)");

    // The rejecting direction: kColor's rows 3, 5 and 7 are fully given, so they
    // are the rows stage 0 must drop. This is the only case here that fails if
    // the "has an unset cell" filter is removed.
    Solver color = make_solver(kColor);
    check(eq(complete_move(color, ""), {"1","2","4","6","8","9"}),
          "kColor: fully-given rows 3, 5 and 7 are not offered");
}

void test_stage1_columns() {
    std::cout << "[complete] stage 1: columns whose cell in the row is unset\n";
    Solver empty = make_solver(kEmpty);
    check(eq(complete_move(empty, "1"), {"11","12","13","14","15","16","17","18","19"}),
          "empty board, row 1: all nine columns offered");

    // kMed row 1 = "3........": column 1 is set (the clue 3), columns 2-9 unset.
    Solver med = make_solver(kMed);
    check(eq(complete_move(med, "1"), {"12","13","14","15","16","17","18","19"}),
          "kMed row 1: the set clue's column (1) is excluded");

    // kColor row 1 = "289...375": only columns 4, 5 and 6 are still unset.
    Solver color = make_solver(kColor);
    check(eq(complete_move(color, "1"), {"14","15","16"}),
          "kColor row 1: only the three unset columns are offered");

    // A row with nothing left in it offers nothing -- the row-level counterpart
    // of the set-cell case in stage 2. Reachable by typing a row digit that
    // stage 0 would not have offered.
    check(complete_move(color, "3").empty(),
          "kColor row 3 is fully given -> no columns offered");
}

void test_stage2_values() {
    std::cout << "[complete] stage 2: candidate values legal in the cell\n";
    Solver empty = make_solver(kEmpty);
    check(eq(complete_move(empty, "11"), {"111","112","113","114","115","116","117","118","119"}),
          "empty board, cell (1,1): all nine values are candidates");

    // In kMed, cell (2,4) is a naked single: its only remaining candidate is 2
    // (the [NS] {[2,4]#2} the solver reports on load). So exactly one value.
    Solver med = make_solver(kMed);
    auto vals = complete_move(med, "24");
    check(eq(vals, {"242"}),
          "kMed cell (2,4) is a naked single -> only value 2 offered");

    // A cell that is already set has no candidates to offer.
    check(complete_move(med, "11").empty(),
          "kMed cell (1,1) is set -> no values offered");
}

void test_nothing_to_complete() {
    std::cout << "[complete] inert cases: full token and invalid partials\n";
    Solver med = make_solver(kMed);
    check(complete_move(med, "123").empty(),
          "a full 'rcv' has nothing left to complete");
    check(complete_move(med, "0").empty(),
          "a 0 digit is out of the 1-9 range -> no matches");
    check(complete_move(med, "x").empty(),
          "a non-digit partial -> no matches");
    check(complete_move(med, "1x").empty(),
          "a non-digit in the column position -> no matches");
}

} // namespace

int main() {
    test_stage0_rows();
    test_stage1_columns();
    test_stage2_values();
    test_nothing_to_complete();

    std::cout << "----------------------------------------\n";
    if (failures == 0) { std::cout << "completion: all checks passed\n"; return 0; }
    std::cout << "completion: " << failures << " check(s) failed\n";
    return 1;
}
