// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "completion.h"

#include "board.h"
#include "cell.h"

namespace {

// The argument digits are 1-9 throughout (rows, columns and values all share
// that range), so any other character makes the partial unparseable.
bool all_digits_1_9(std::string_view s) {
    for (char c : s) {
        if (c < '1' || c > '9') return false;
    }
    return true;
}

} // namespace

std::vector<std::string> complete_move(const Solver &solver, std::string_view partial) {
    std::vector<std::string> out;

    // A full "rcv" leaves nothing to complete; a non-1-9 character is not a
    // coordinate we can reason about.
    if (partial.size() > 2) return out;
    if (!all_digits_1_9(partial)) return out;

    if (partial.empty()) {
        // Stage 0: every row that still contains at least one unset cell.
        for (size_t r = 0; r < Board::height; ++r) {
            for (size_t c = 0; c < Board::width; ++c) {
                if (solver.is_unset(r, c)) {
                    out.push_back(std::string(1, static_cast<char>('1' + r)));
                    break;
                }
            }
        }
        return out;
    }

    const size_t row = static_cast<size_t>(partial[0] - '1');

    if (partial.size() == 1) {
        // Stage 1: every column whose cell in this row is still unset.
        for (size_t c = 0; c < Board::width; ++c) {
            if (solver.is_unset(row, c)) {
                out.push_back(std::string(partial) + static_cast<char>('1' + c));
            }
        }
        return out;
    }

    // Stage 2 (partial is "rc"): every candidate value still legal at the cell.
    const size_t col = static_cast<size_t>(partial[1] - '1');
    for (Value v : solver.candidates_at(row, col)) {
        out.push_back(std::string(partial) + static_cast<char>('0' + static_cast<int>(v)));
    }
    return out;
}
