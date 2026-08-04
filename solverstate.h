// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "board.h"
#include "analyzer.h"

#include <memory>

class SolverState {
public:
    using ptr = std::unique_ptr<SolverState>;

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

    // Live cell queries for the interactive completer (see completion.h),
    // delegating to Board exactly as edit_note/set_value do for the mutations.
    // Intent-level rather than a `const Board &` accessor, so mBoard stays
    // private. Preconditions are Board's: row and col must be in range.
    bool is_unset(size_t row, size_t col) const { return mBoard.is_unset_at(row, col); }
    ValueList candidates_at(size_t row, size_t col) const { return mBoard.candidates_at(row, col); }

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
