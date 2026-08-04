// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "solverstate.h"

#include <memory>

class Solver {
public:
    using ptr = std::unique_ptr<Solver>;

    Solver(const std::string &board_desc) {
        mStates.push_back(std::make_unique<SolverState>(board_desc));
    }

    bool solve_one_step(bool singles_only);
    bool solve();
    bool solve_singles();
    bool back_one_step();
    bool reset();
    bool edit_note(const std::string &);
    bool set_value(const std::string &);
    void print_current_state(std::ostream &outs) const {
        if (!mStates.empty()) {
            mStates.back()->print(outs);
        }
    }
    void print_candidates(std::ostream &outs) const {
        if (!mStates.empty()) {
            mStates.back()->print_candidates(outs);
        }
    }

    bool solved() const { return mStates.back()->solved(); }

    // --- accessors for interactive tab completion (see completion.h) ---
    // These delegate down to SolverState and then Board, the same chain
    // edit_note/set_value take for the mutating side. row and col must be in
    // range: Board asserts it, as it does for the '=' and 'x' edits.
    //
    // Is the cell at (row,col) currently unset (still a note cell)?
    bool is_unset(size_t row, size_t col) const;
    // The candidate values still legal at (row,col), ascending. Empty for a cell
    // that is already set.
    ValueList candidates_at(size_t row, size_t col) const;

    friend std::ostream &operator<<(std::ostream &, const Solver &);

private:
    std::vector<SolverState::ptr> mStates;
};
