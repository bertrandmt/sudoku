// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "board.h"
#include "cell.h"

#include <iostream>
#include <string>

namespace {

void help(void) {
    std::cout << "Enter table entries in the format:" << std::endl
              << "    rcv[;...]" << std::endl
              << "  where:" << std::endl
              << "    * \"r\" is the row, with value between 1 and 9, and" << std::endl
              << "    * \"c\" is the column, with value between 1 and 9, and" << std::endl
              << "    * \"v\" is the value, with value between 1 and 9." << std::endl
              << "Several entries on one line, separated by semicolons, are allowed." << std::endl
              << std::endl
              << "Other commands:" << std::endl
              << "  '%'     run one step of auto-solving" << std::endl;
}

bool record_entry(Board &board, const std::string &entry) {
    if (entry.size() != 3) {
        help();
        return false;
    }

    size_t row = 0;
    switch (entry[0]) {
        case '1': row = 0; break;
        case '2': row = 1; break;
        case '3': row = 2; break;
        case '4': row = 3; break;
        case '5': row = 4; break;
        case '6': row = 5; break;
        case '7': row = 6; break;
        case '8': row = 7; break;
        case '9': row = 8; break;
        default: help(); return false;
    }
    size_t col = 0;
    switch (entry[1]) {
        case '1': col = 0; break;
        case '2': col = 1; break;
        case '3': col = 2; break;
        case '4': col = 3; break;
        case '5': col = 4; break;
        case '6': col = 5; break;
        case '7': col = 6; break;
        case '8': col = 7; break;
        case '9': col = 8; break;
        default: help(); return false;
    }
    Value val = kUnset;
    switch (entry[2]) {
        case '1': val = kOne; break;
        case '2': val = kTwo; break;
        case '3': val = kThree; break;
        case '4': val = kFour; break;
        case '5': val = kFive; break;
        case '6': val = kSix; break;
        case '7': val = kSeven; break;
        case '8': val = kEight; break;
        case '9': val = kNine; break;
        default: help(); return false;
    }

    board.at(row, col) = val;
    return true;
}

void record_entries(Board &board, const std::string &entries) {
    std::vector<std::string> v_entries;
    size_t ofsb = 0, ofse = 0;
    while (ofse != std::string::npos) {
        ofse = entries.find(';', ofsb);
        std::string entry = entries.substr(ofsb, ofse - ofsb);
        v_entries.push_back(entry);
        ofsb = ofse + 1;
    }

    for (auto const &entry : v_entries) {
        bool ok = record_entry(board, entry);
        if (!ok) return;
    }

    board.autonote();
    std::cout << board << std::endl;
}

void autosolve_one_step(Board &board) {
    if (board.naked_single()) {
        std::cout << board << std::endl;
        return;
    }

    if (board.hidden_single()) {
        std::cout << board << std::endl;
        return;
    }

    std::cout << "No autosolve step made progress :(" << std::endl;
}

bool routine(Board &board) {
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

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': // it's recording one or more entries
            record_entries(board, nowsline);
            break;

        case '%': // auto-solve one step
            autosolve_one_step(board);
            break;

        default:
            help();
            break;
    }

    return done;
}

} // namespace anonymous

int main(void) {
    Board board;
    bool done = false;

    do {
        done = routine(board);
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
