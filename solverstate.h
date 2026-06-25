// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "analyzer.h"

#include <memory>

class SolverState {
public:
    using ptr = std::shared_ptr<SolverState>;

    SolverState(const std::string &board_desc)
        : mBoard(board_desc)
        , mAnalyzer(mBoard)
        , mGeneration(0) {
        mAnalyzer.analyze();
    }

    SolverState(const SolverState &other)
        : mBoard(other.mBoard)
        , mAnalyzer(mBoard, other.mAnalyzer)
        , mGeneration(other.mGeneration + 1) {
        }

    size_t generation() const { return mGeneration; }

    bool act(const bool);
    bool edit_note(const std::string &);
    bool set_value(const std::string &);

    bool solved() const { return mBoard.note_cells_count() == 0; }

    // Read-only view of the current board, used by the interactive completer to
    // read live cell state (see completion.h). The REPL reaches this through
    // Solver's intent-level accessors rather than touching the board directly.
    const Board &board() const { return mBoard; }

    void print(std::ostream &outs) const {
        mBoard.print(outs);
    }
    void print_candidates(std::ostream &outs) const {
        mBoard.print_candidates(outs);
    }
    friend std::ostream &operator<<(std::ostream &, const SolverState &);

private:
    Board mBoard;
    Analyzer mAnalyzer;
    size_t mGeneration;
};
