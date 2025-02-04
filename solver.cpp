// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "solver.h"

bool Solver::solve_one_step(bool singles_only) {
    if (mStates.back()->solved()) return false;

    SolverState::ptr nextState(new SolverState(*mStates.back()));
    std::cout << "Step #" << nextState->generation() << ":" << std::endl;

    if (nextState->act(singles_only)) {
        mStates.push_back(nextState);

        if (nextState->solved()) {
            std::cout << "SOLVED!" << std::endl;
        }
        return true;
    }

    return false;
}

bool Solver::solve() {
    if (mStates.back()->solved()) return false;

    bool did_act = false;
    while (solve_one_step(false)) did_act = true;

    if (!mStates.back()->solved()) {
        std::cout << "???" << std::endl;
    }

    return did_act;
}

bool Solver::solve_singles() {
    if (mStates.back()->solved()) return false;

    bool did_act = false;
    while (solve_one_step(true)) did_act = true;

    if (!mStates.back()->solved()) {
        std::cout << "???" << std::endl;
    }

    return did_act;
}

bool Solver::back_one_step() {
    bool did_act = false;
    if (mStates.size() > 1) {
        mStates.pop_back();
        did_act = true;
        std::cout << "Step #" << mStates.back()->generation() << ":" << std::endl;
    }
    return did_act;
}

bool Solver::reset() {
    bool did_act = false;
    if (mStates.size() > 1) {
        mStates.erase(mStates.begin() + 1, mStates.end());
        did_act = true;
        std::cout << "Step #" << mStates.back()->generation() << ":" << std::endl;
    }
    return did_act;
}

bool Solver::edit_note(const std::string &entry) {
    SolverState::ptr nextState(new SolverState(*mStates.back()));

    bool did_act = false;

    if (nextState->edit_note(entry)) {
        did_act = true;
        std::cout << "Step #" << nextState->generation() << ":" << std::endl;
    }

    if (did_act) mStates.push_back(nextState);
    return did_act;
}

bool Solver::set_value(const std::string &entry) {
    SolverState::ptr nextState(new SolverState(*mStates.back()));

    bool did_act = false;

    if (nextState->set_value(entry)) {
        did_act = true;
        std::cout << "Step #" << nextState->generation() << ":" << std::endl;
    }

    if (did_act) mStates.push_back(nextState);
    return did_act;
}

std::ostream &operator<<(std::ostream &outs, const Solver &solver) {
    return outs << *solver.mStates.back();
}
