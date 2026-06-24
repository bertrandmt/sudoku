// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "solverstate.h"

bool SolverState::act(const bool singles_only) {
    bool did_act = mAnalyzer.act(singles_only);
    if (did_act) {
        mAnalyzer.analyze();
    }
    return did_act;
}

bool SolverState::edit_note(const std::string &entry) {
    size_t row, col;
    Value val;
    if (!parse_rcv(entry, row, col, val)) { return false; }

    bool did_clear_note = mBoard.clear_note_at(row, col, val);
    if (did_clear_note) {
        mAnalyzer.analyze();
    }
    return did_clear_note;
}

bool SolverState::set_value(const std::string &entry) {
    size_t row, col;
    Value val;
    if (!parse_rcv(entry, row, col, val)) { return false; }

    bool did_set_value = mBoard.set_value_at(row, col, val);
    if (did_set_value) {
        mAnalyzer.analyze();
    }
    return did_set_value;
}

std::ostream &operator<<(std::ostream &outs, const SolverState &state) {
    outs << state.mBoard;
    if (!state.solved()) {
        outs << std::endl
             << state.mAnalyzer;
    }
    return outs;
}
