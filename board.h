// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include "cell.h"
#include "analyzer.h"

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
    Board(Analyzer &analyzer, const std::string &board_desc);
private:
    void record_entries_form1(const std::string &);
    void record_entry_form1(const std::string &);
    void record_entries_form2(const std::string &);
public:
    Board(Analyzer &analyzer, const Board &other);

    using ptr = std::shared_ptr<Board>;

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

    std::vector<Cell> &cells() { return mCells; }
    const std::vector<Cell> &cells() const { return mCells; }

    std::vector<Row> &rows() { return mRows; }
    const std::vector<Row> &rows() const { return mRows; }

    std::vector<Column> &columns() { return mColumns; }
    const std::vector<Column> &columns() const { return mColumns; }

    std::vector<Nonet> &nonets() { return mNonets; }
    const std::vector<Nonet> &nonets() const { return mNonets; }

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

    Analyzer &mAnalyzer;

    Cell &at(size_t row, size_t col);
    const Cell &at(size_t row, size_t col) const;
    Cell &at(const Coord &coord) { return at(coord.row(), coord.column()); }
    const Cell &at(const Coord &coord) const { return at(coord.row(), coord.column()); }

    void rebuild_subsets();
};

#include "row.h"
#include "column.h"
#include "nonet.h"
