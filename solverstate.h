// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"

#include <memory>

class SolverState {
public:
    using ptr = std::shared_ptr<SolverState>;

    SolverState(const std::string &board_desc)
        : mBoard(new Board(board_desc))
        , mGeneration(0) { }

    SolverState(const SolverState &other)
        : mBoard(new Board(*other.mBoard))
        , mGeneration(other.mGeneration + 1) { }

    size_t generation() const { return mGeneration; }

    bool act(const bool);
    bool edit_note(const std::string &);
    bool set_value(const std::string &);

    void print(std::ostream &outs) const {
        mBoard->print(outs);
    }
    friend std::ostream &operator<<(std::ostream &, const SolverState &);

private:
    Board::ptr mBoard;
    size_t mGeneration;
};
