// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License
#pragma once

#include <functional>
#include <array>
#include <cassert>
#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <ranges>

#include "coord.h"

enum Value: int {
    kOne = 1,
    kTwo,
    kThree,
    kFour,
    kFive,
    kSix,
    kSeven,
    kEight,
    kNine
};

// Iterate the nine digit values kOne..kNine. The transform turns the integer
// iota back into Value; iota's half-open [begin, end) is why the end is one
// past kNine.
inline auto value_range() {
    return std::views::iota(static_cast<int>(kOne), static_cast<int>(kNine) + 1)
         | std::views::transform([](int i) { return static_cast<Value>(i); });
}

class Cell;

// The candidates of a Notes, materialized inline with no heap allocation. A cell
// holds at most nine candidates, so a fixed nine-slot array plus a count covers
// every case while staying trivially copyable. This is a drop-in for the
// std::vector<Value> that Notes::values() used to return, exposing exactly what
// the analyzer call sites need: range iteration, indexing, and size. (Set
// equality of two cells' candidates goes through Notes::operator== on the
// bitmask, so ValueList itself needs no equality.)
class ValueList {
public:
    using const_iterator = const Value *;

    void push_back(const Value &v) { assert(mCount < mData.size()); mData[mCount++] = v; }

    size_t size() const { return mCount; }

    Value operator[](size_t i) const { assert(i < mCount); return mData[i]; }
    Value at(size_t i) const { return (*this)[i]; }

    const_iterator begin() const { return mData.data(); }
    const_iterator end() const { return mData.data() + mCount; }

private:
    std::array<Value, 9> mData {};
    size_t mCount = 0;
};

// The nine candidate flags packed into a bitmask: bit (v - 1) is set when v is
// still a candidate. A plain integer keeps Notes (and therefore Cell) trivially
// copyable, with no per-cell heap allocation.
class Notes {
public:
    Notes()
        : mNotes(kAllCandidates) { }

    void clear() { mNotes = 0; }

    bool check(const Value &v) const { return (mNotes & bit(v)) != 0; }
    bool set(const Value &v, bool set);
    bool set_all(bool set) { mNotes = set ? kAllCandidates : 0; return true; }

    // Two note sets are equal iff they hold the same candidates. The bitmask is
    // the canonical set representation, so this is one integer compare -- order-
    // blind and independent of how the candidates were set.
    bool operator==(const Notes &other) const { return mNotes == other.mNotes; }

    size_t count() const { return std::popcount(mNotes); }
    ValueList values() const;

    // For a two-candidate cell, the candidate that isn't v. Clearing v's bit
    // leaves a single bit standing; its position (0-based) is the value minus 1.
    Value other_value(const Value &v) const {
        assert(count() == 2);
        assert(check(v));
        return static_cast<Value>(std::countr_zero<uint16_t>(mNotes & ~bit(v)) + 1);
    }

private:
    static constexpr uint16_t bit(const Value &v) { return static_cast<uint16_t>(1u << (v - 1)); }
    static constexpr uint16_t kAllCandidates = 0x1ffu; // bits 0..8 -> values 1..9

    uint16_t mNotes;
};

class Cell {
public:
    Cell(size_t line, size_t column)
        : mCoord(line, column)
        , mValue(std::nullopt) { }

    bool isNote() const { return !mValue.has_value(); }
    bool isValue() const { return mValue.has_value(); }

    const Coord &coord() const { return mCoord; }
    Value value() const { assert(isValue()); return *mValue; }
    Notes &notes() { assert(isNote()); return mNotes; }
    const Notes &notes() const { assert(isNote()); return mNotes; }
    Value other_value(const Value &value) const {
        return notes().other_value(value);
    }

    bool check(const Value &v) const { return isNote() && mNotes.check(v); }
    bool set(const Value &v, bool set) { return isNote() && mNotes.set(v, set); }
    bool set_all(bool set) { return isNote() && mNotes.set_all(set); }
    void set(const Value &v);

    bool operator==(const Cell &other) const {
        return mCoord == other.mCoord;
    }

    // Render one of the three display lines (row 0..2) of this cell's 3x3
    // candidate grid into outs. The caller drives the three lines in sequence;
    // keeping the line index a parameter (rather than internal state) makes the
    // cell const-correct and the rendering reentrant.
    void print_row(std::ostream &outs, size_t row) const;

private:
    const Coord mCoord;

    std::optional<Value> mValue;
    Notes mNotes;
};

// Copies of Cell happen in the analyzer's inner loop (candidates()) and in the
// per-solve-step SolverState copy; keeping copy construction trivial means those
// copies are a plain memcpy with no heap traffic. We assert *copy construction*
// rather than full is_trivially_copyable because the const mCoord intentionally
// deletes copy assignment (Cell is copy-constructible but not assignable), and a
// deleted assignment is enough to make is_trivially_copyable false.
static_assert(std::is_trivially_copy_constructible_v<Cell>,
              "Cell copies must stay a trivial memcpy (no per-cell heap allocation)");
static_assert(std::is_trivially_destructible_v<Cell>,
              "Cell must stay trivially destructible");

template<>
struct std::hash<Cell> {
    std::size_t operator()(const Cell &cell) const noexcept;
};
