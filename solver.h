// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "solverstate.h"

#include <memory>

class Solver {
public:
    using ptr = std::shared_ptr<Solver>;

    Solver()
        : mValid(false) { }

    Solver(const std::string &board_desc) {
        SolverState::ptr initial_state(new SolverState(board_desc));
        mStates.push_back(initial_state);

        mValid = initial_state->valid();
    }

    bool valid() const { return mValid; }

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

    friend std::ostream &operator<<(std::ostream &, const Solver &);

private:
    bool mValid;
    std::vector<SolverState::ptr> mStates;
};
