// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "solver.h"

#include <string>
#include <string_view>
#include <vector>

// Pure, testable core of the interactive tab completion for the '=' (set value)
// and 'x' (strike note) commands. Both verbs take the same "rcv" argument, so a
// single generator drives both; the verb is reattached by the readline glue.
//
// Given the live solver and the digits already typed after the verb (0, 1 or 2
// of them), return the complete argument tokens that are legal next, ascending:
//   ""  (0 digits) -> "r"   for each row 1-9 that still has an unset cell
//   "r" (1 digit)  -> "rc"  for each column c where cell (r,c) is unset
//   "rc"(2 digits) -> "rcv" for each candidate value v legal at (r,c)
// Each returned string is the full argument (digits only, no verb), so the
// caller prepends the verb to form the replacement word.
//
// Returns empty when there is nothing useful to offer: a partial that is
// already a full "rcv", a non-digit or out-of-1-9 character, or a row/cell that
// has no unset cells / no candidates left.
std::vector<std::string> complete_move(const Solver &solver, std::string_view partial);
