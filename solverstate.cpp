// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "solverstate.h"

bool SolverState::act(const bool singles_only) {
    if (!isValid()) return false;
    return mBoard.act(singles_only);
}

bool SolverState::edit_note(const std::string &entry) {
    if (entry.size() != 3) { return false; }

    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) { return false; }

    Cell &c = mBoard.at(row, col);
    if (!c.isNote()) { return false; }
    if (!c.check(val)) { return false; }

    c.set(val, false);

    mBoard.analyze(c);

    return true;
}

bool SolverState::set_value(const std::string &entry) {
    if (entry.size() != 3) { return false; }

    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) { return false; }

    Cell &c = mBoard.at(row, col);
    if (!c.isNote()) { return false; }

    c.set(val);

    mBoard.autonote(c);
    mBoard.analyze(c);

    return true;
}

bool SolverState::record_entry_form1(const std::string &entry) {
    if (entry.size() != 3) { return false; }

    size_t row = entry[0] - '1';
    size_t col = entry[1] - '1';
    Value val = static_cast<Value>(entry[2] - '0');
    if (val == kUnset) { return false; }

    mBoard.at(row, col).set(val);
    return true;
}

bool SolverState::record_entries_form1(const std::string &entries) {
    size_t ofsb = 0, ofse = 0;
    while (ofse != std::string::npos) {
        ofse = entries.find(';', ofsb);
        std::string entry = entries.substr(ofsb, ofse - ofsb);
        if (!record_entry_form1(entry)) return false;
        ofsb = ofse + 1;
    }
    return true;
}

bool SolverState::record_entries_form2(const std::string &entries) {
    if (entries.size() != mBoard.width * mBoard.height) {
        return false;
    }

    size_t index = 0;
    for (auto &c : mBoard.cells()) {
        switch (entries[index]) {
        case '.': // it's a note entry
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': // it's a value entry
            c.set(static_cast<Value>(entries[index] - '0'));
            break;

        default: // don't know what to do with this
            return false;
        }
        index++;
    }

    return true;
}

std::ostream &operator<<(std::ostream &outs, const SolverState &state) {
    assert(state.mValid);

    return outs << state.mBoard;
}
