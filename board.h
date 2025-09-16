// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"

#include <vector>
#include <unordered_set>
#include <iterator>
#include <cstddef>
#include <memory>

class Row;
class Column;
class Nonet;


class Board {
public:
    friend class Analyzer;
    Board(const std::string &board_desc);
private:
    void record_entries_form1(const std::string &);
    void record_entry_form1(const std::string &);
    void record_entries_form2(const std::string &);
public:
    Board(const Board &other);

    static const size_t width = 9;
    static const size_t height = 9;

    void print(std::ostream &out) const;

    bool clear_note_at(const Coord &, const Value &);
    bool clear_note_at(size_t row, size_t col, const Value &);
    bool set_value_at(const Coord &, const Value &);
    bool set_value_at(size_t row, size_t col, const Value &);

    const Row &row(const Cell &) const;
    const Column &column(const Cell &) const;
    const Nonet &nonet(const Cell &) const;

    const Row &row(const Coord &) const;
    const Column &column(const Coord &) const;
    const Nonet &nonet(const Coord &) const;

    const std::vector<Cell> &cells() const { return mCells; }
    std::vector<Cell> &cells() { return mCells; }

    const std::vector<Row> &rows() const { return mRows; }
    const std::vector<Column> &columns() const { return mColumns; }
    const std::vector<Nonet> &nonets() const { return mNonets; }

    // are these two coords in the same row, column or nonet?
    bool see_each_other(const Coord &, const Coord &, std::string &out_tag) const;
    bool see_each_other(const Cell &c1, const Cell &c2) const {
        std::string tag;
        return see_each_other(c1.coord(), c2.coord(), tag);
    }

    // do any two of the passed coords see each other?
    bool any_see_each_other(const std::vector<Coord> &, std::string &out_tag) const;

    friend std::ostream& operator<< (std::ostream& outs, const Board &);
    friend class Row;
    friend class Column;
    friend class Nonet;

    size_t note_cells_count() const { return mNoteCellsCount; }

private:
    std::vector<Cell> mCells;
    std::vector<Row> mRows;
    std::vector<Column> mColumns;
    std::vector<Nonet> mNonets;

    size_t mNoteCellsCount;
    size_t mNotesCount;

    Cell &at(size_t row, size_t col);
    const Cell &at(size_t row, size_t col) const;
    Cell &at(const Coord &coord) { return at(coord.row(), coord.column()); }
    const Cell &at(const Coord &coord) const { return at(coord.row(), coord.column()); }

    void rebuild_subsets();
};

#include "row.h"
#include "column.h"
#include "nonet.h"
