// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "solverstate.h"

bool SolverState::act(const bool singles_only) {
    return mBoard->act(singles_only);
}

bool SolverState::edit_note(const std::string &entry) {
    if (entry.size() != 3) { return false; }

    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) { return false; }

    return mBoard->clear_note_at(row, col, val);
}

bool SolverState::set_value(const std::string &entry) {
    if (entry.size() != 3) { return false; }

    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) { return false; }

    return mBoard->set_value_at(row, col, val);
}

std::ostream &operator<<(std::ostream &outs, const SolverState &state) {
    return outs << *state.mBoard;
}
