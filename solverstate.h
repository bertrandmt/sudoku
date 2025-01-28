// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"

#include <memory>

class SolverState {
public:
    using ptr = std::shared_ptr<SolverState>;

    SolverState(const std::string &board_desc)
        : mValid(false)
        , mGeneration(0) {

        switch(board_desc[0]) {
        case ';':
            if (!record_entries_form1(board_desc.substr(1))) return;
            break;
        case '.':
            if (!record_entries_form2(board_desc.substr(1))) return;
            break;
        default:
            // don't know how to parse this
            return;
        }

        mBoard.autonote();
        mValid = true;
    }

    SolverState(const SolverState &other)
        : mValid(other.mValid)
        , mBoard(other.mBoard)
        , mGeneration(other.mGeneration + 1) { }

    bool isValid() const { return mValid; }
    const Board &board() const { return mBoard; }
    Board &board() { return mBoard; }
    size_t generation() const { return mGeneration; }

    bool edit_note(const std::string &);

    friend std::ostream &operator<<(std::ostream &, const SolverState &);

private:
    bool mValid;
    Board mBoard;
    size_t mGeneration;

    bool record_entries_form1(const std::string &);
    bool record_entry_form1(const std::string &);
    bool record_entries_form2(const std::string &);
};
