// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "cell.h"
#include "solverstate.h"
#include "solver.h"

#include <iostream>
#include <string>

namespace {

void help(void) {
    std::cout << "Commands:" << std::endl
              << "  'n'          Start a new game" << std::endl
              << "               Enter table entries in one of two formats:" << std::endl
              << "                 ;rcv[;...]" << std::endl
              << "                   where:" << std::endl
              << "                     * \"r\" is the row, with value between 1 and 9, and" << std::endl
              << "                     * \"c\" is the column, with value between 1 and 9, and" << std::endl
              << "                     * \"v\" is the value, with value between 1 and 9." << std::endl
              << "                   Enter all entries, on one line, separated by semicolons." << std::endl
              << "                 .[v|.]*" << std::endl
              << "                   where:" << std::endl
              << "                     * \"v\" is a value between 1 and 9, and" << std::endl
              << "                     * \".\" indicates an unset cell." << std::endl
              << "                   All 81 cells in a board must be entered." << std::endl
              << std::endl
              << "Other commands:" << std::endl
              << "  '>' or '.'    run one step of auto-solving" << std::endl
              << "  '<' or ','    go back one auto-solveing step" << std::endl
              << "  '!'           reset the solver to its initial state" << std::endl
              << "  'r'           run auto-solving until blocked (or done)" << std::endl
              << "  's'           run auto-solving using only 'singles' heuristics" << std::endl
              << "  'xrcv'        edit note at row 'r' and column 'c' and unset value 'v'" << std::endl
              << "  'p'           print the board in a compact format" << std::endl;
}

bool routine(Solver::ptr &solver) {
    bool done = false;

    std::string line;
    std::cout << "λ " << std::flush;
    std::getline(std::cin, line);
    if (!std::cin) {
        std::cout << std::endl;
        done = true;
        return done;
    }

    // no whitespace we care to make use of
    std::string nowsline(line, 0);
    nowsline.erase(
            std::remove_if(nowsline.begin(), nowsline.end(), [](auto c){ return std::isspace(c); }),
            nowsline.end());
    if (nowsline.size() == 0) { return done; }

    switch (nowsline[0]) {
        case '#': // it's a comment
            std::cout << line << std::endl;
            return done;

        case 'n': { // it's a new game
            Solver::ptr s(new Solver(nowsline.substr(1)));
            if (!s->isValid()) {
                help();
                break;
            }
            solver = s;
            std::cout << *solver << std::endl;
            }
            break;

        case '.':
        case '>': // auto-solve one step
            if (!solver->isValid()) { help(); break; }
            if (solver->solve_one_step(false)) { std::cout << *solver << std::endl; }
            else                               { std::cout << "???" << std::endl; }
            break;

        case ',':
        case '<': // back one step
            if (!solver->isValid()) { help(); break; }
            if (solver->back_one_step()) { std::cout << *solver << std::endl; }
            break;

        case 'r':
        case 'R': // auto-solve until blocked (or finished)
            if (!solver->isValid()) { help(); break; }
            if (solver->solve()) { std::cout << *solver << std::endl; }
            break;

        case 's':
        case 'S':
            if (!solver->isValid()) { help(); break; }
            if (solver->solve_singles()) { std::cout << *solver << std::endl; }
            break;

        case '!': // reset
            if (!solver->isValid()) { help(); break; }
            if (solver->reset()) { std::cout << *solver << std::endl; }
            break;

        case 'x':
        case 'X': // edit a note
            if (!solver->isValid()) { help(); break; }
            if (solver->edit_note(nowsline.substr(1))) { std::cout << *solver << std::endl; }
            else                                       { help(); }
            break;

        case 'p':
        case 'P':
            if (!solver->isValid()) { help(); break; }
            solver->currentState()->board().print(std::cout);
            break;

        default:
            help();
            break;
    }

    return done;
}

} // namespace anonymous

int main(void) {
    Solver::ptr solver(new Solver());

    bool done = false;

    do {
        done = routine(solver);
    } while (!done);

    return 0;
}

#if 0
    {
        Board b;

        b.at(0, 1) = kOne;
        b.at(0, 3) = kNine;
        b.at(0, 5) = kEight;
        b.at(0, 6) = kSeven;

        b.at(1, 0) = kEight;
        b.at(1, 8) = kThree;

        b.at(2, 1) = kNine;
        //b.at(2, 6) = kTwo; // TO BE REMOVED

        b.at(3, 5) = kFive;

        b.at(4, 0) = kSeven;
        b.at(4, 1) = kThree;
        b.at(4, 3) = kTwo;
        b.at(4, 5) = kSix;
        b.at(4, 6) = kEight;
        b.at(4, 8) = kNine;

        b.at(5, 1) = kTwo;
        b.at(5, 3) = kEight;
        b.at(5, 4) = kSeven;
        b.at(5, 6) = kSix;
        b.at(5, 7) = kFive;

        b.at(6, 3) = kOne;
        b.at(6, 8) = kFive;

        b.at(7, 0) = kTwo;
        b.at(7, 2) = kSix;

        b.at(8, 2) = kNine;
        b.at(8, 7) = kSix;

        // a.k.a 121;149;168;177;218;293;329;465;517;523;542;566;578;599;622;648;657;676;685;741;795;812;836;939;986

        b.autonote();
        std::cout << b << std::endl;
#if 0
        for (auto pr = b.row_begin(); pr != b.row_end(); ++pr) {
            std::cout << *pr << std::endl;
        }

        Column col(b, 2);
        std::cout << col << std::endl;

        for (auto pc = b.column_begin(); pc != b.column_end(); ++pc) {
            std::cout << *pc << std::endl;
        }

        Coord c(5, 1);
        Nonet n(b, c);
        std::cout << n << std::endl;

        for (auto pn = b.nonet_begin(); pn != b.nonet_end(); ++pn) {
            std::cout << *pn << std::endl;
        }

        b.autonote(0, 1);
#endif

        bool state_changed;
        do {
            state_changed = false;
            if (b.naked_single()) {
                state_changed = true;
                std::cout << b << std::endl;
            }

            if (b.hidden_single()) {
                state_changed = true;
                std::cout << b << std::endl;
            }
        } while (state_changed);
    }
#endif

#if 0
    {
        Cell c(Cell::kThree);
        std::cout << c.toString() << std::endl;
    }
    {
        Board b;
        b.at(0, 0) = Cell::kOne;
        b.at(0, 1) = Cell::kTwo;
        b.at(0, 2) = Cell::kThree;
        b.at(0, 3) = Cell::kFour;
        b.at(0, 4) = Cell::kFive;
        b.at(0, 5) = Cell::kSix;
        b.at(0, 6) = Cell::kSeven;
        b.at(0, 7) = Cell::kEight;
        b.at(0, 8) = Cell::kNine;

        b.at(1, 0).setNote(Cell::kOne);

        b.at(1, 1).setNote(Cell::kOne);
        b.at(1, 1).setNote(Cell::kTwo);

        b.at(1, 2).setNote(Cell::kOne);
        b.at(1, 2).setNote(Cell::kTwo);
        b.at(1, 2).setNote(Cell::kThree);

        b.at(1, 3).setNote(Cell::kOne);
        b.at(1, 3).setNote(Cell::kTwo);
        b.at(1, 3).setNote(Cell::kThree);
        b.at(1, 3).setNote(Cell::kFour);

        b.at(1, 4).setNote(Cell::kOne);
        b.at(1, 4).setNote(Cell::kTwo);
        b.at(1, 4).setNote(Cell::kThree);
        b.at(1, 4).setNote(Cell::kFour);
        b.at(1, 4).setNote(Cell::kFive);

        b.at(1, 5).setNote(Cell::kSix);

        b.at(1, 6).setNote(Cell::kSeven);

        b.at(1, 7).setNote(Cell::kEight);

        b.at(1, 8).setNote(Cell::kNine);

        std::cout << b.toString() << std::endl;
    }
#endif
