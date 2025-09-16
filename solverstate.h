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

    void print(std::ostream &outs) const {
        mBoard.print(outs);
    }
    friend std::ostream &operator<<(std::ostream &, const SolverState &);

private:
    Board mBoard;
    Analyzer mAnalyzer;
    size_t mGeneration;
};
