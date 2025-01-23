// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "cell.h"

#include <iostream>

int main(void) {
    {
        Board b;

        b.at(0, 1) = Cell::kOne;
        b.at(0, 3) = Cell::kNine;
        b.at(0, 5) = Cell::kEight;
        b.at(0, 6) = Cell::kSeven;

        b.at(1, 0) = Cell::kEight;
        b.at(1, 8) = Cell::kThree;

        b.at(2, 1) = Cell::kNine;

        b.at(3, 5) = Cell::kFive;

        b.at(4, 0) = Cell::kSeven;
        b.at(4, 1) = Cell::kThree;
        b.at(4, 3) = Cell::kTwo;
        b.at(4, 5) = Cell::kSix;
        b.at(4, 6) = Cell::kEight;
        b.at(4, 8) = Cell::kNine;

        b.at(5, 1) = Cell::kTwo;
        b.at(5, 3) = Cell::kEight;
        b.at(5, 4) = Cell::kSeven;
        b.at(5, 6) = Cell::kSix;
        b.at(5, 7) = Cell::kFive;

        b.at(6, 3) = Cell::kOne;
        b.at(6, 8) = Cell::kFive;

        b.at(7, 0) = Cell::kTwo;
        b.at(7, 2) = Cell::kSix;

        b.at(8, 2) = Cell::kNine;
        b.at(8, 7) = Cell::kSix;

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

        b.autonote();
        std::cout << b << std::endl;
    }

    return 0;
}

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
