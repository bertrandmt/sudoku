// Copyright (c) 2025, Bertrand Mollinier Toublet
// See LICENSE for details of BSD 3-Clause License

#include "analyzer.h"
#include "board.h"
#include "verbose.h"
#include <cassert>
#include <queue>

namespace {
    // Helper function to find strong links (exactly 2 candidates for a value in a unit)
    template<class Set>
    std::vector<Cell> find_strong_link_candidates(const Set &set, const Value &value) {
        std::vector<Cell> candidates;
        for (auto const &cell : set) {
            if (cell.isNote() && cell.check(value)) {
                candidates.push_back(cell);
            }
        }
        return candidates.size() == 2 ? candidates : std::vector<Cell>();
    }
    
    // Check if two cells form a strong link for a value
    template<class Board>
    bool forms_strong_link(Board *board, const Cell &cell1, const Cell &cell2, const Value &value) {
        // Check if they're in the same row and it has exactly 2 candidates
        if (board->row(cell1) == board->row(cell2)) {
            auto candidates = find_strong_link_candidates(board->row(cell1), value);
            if (candidates.size() == 2) {
                return (candidates[0] == cell1 && candidates[1] == cell2) ||
                       (candidates[0] == cell2 && candidates[1] == cell1);
            }
        }
        
        // Check if they're in the same column and it has exactly 2 candidates  
        if (board->column(cell1) == board->column(cell2)) {
            auto candidates = find_strong_link_candidates(board->column(cell1), value);
            if (candidates.size() == 2) {
                return (candidates[0] == cell1 && candidates[1] == cell2) ||
                       (candidates[0] == cell2 && candidates[1] == cell1);
            }
        }
        
        // Check if they're in the same nonet and it has exactly 2 candidates
        if (board->nonet(cell1) == board->nonet(cell2)) {
            auto candidates = find_strong_link_candidates(board->nonet(cell1), value);
            if (candidates.size() == 2) {
                return (candidates[0] == cell1 && candidates[1] == cell2) ||
                       (candidates[0] == cell2 && candidates[1] == cell1);
            }
        }
        
        return false;
    }
}

bool Analyzer::test_coloring_graph(const ColoringGraph &graph) const {
    // A coloring graph is valid if:
    // 1. All cells in the graph still have the candidate value
    // 2. The graph represents a valid coloring (connected via strong links)
    
    if (graph.size() < 2) return false;
    
    for (const auto &[coord, color] : graph) {
        const Cell &cell = mBoard->at(coord);
        if (!cell.isNote() || !cell.check(graph.value)) {
            return false;
        }
    }
    
    // Check that the graph is still connected via strong links
    // Each cell must have at least one strong link to another cell in the graph
    for (const auto &[coord, color] : graph) {
        const Cell &current_cell = mBoard->at(coord);
        bool has_strong_link = false;
        
        for (const auto &[other_coord, other_color] : graph) {
            if (coord == other_coord) continue;
            
            const Cell &other_cell = mBoard->at(other_coord);
            if (forms_strong_link(mBoard, current_cell, other_cell, graph.value)) {
                has_strong_link = true;
                break;
            }
        }
        
        if (!has_strong_link) return false;
    }
    
    return true;
}

void Analyzer::filter_coloring_graphs() {
    mColoringGraphs.erase(std::remove_if(mColoringGraphs.begin(), mColoringGraphs.end(),
                [this](const auto &graph) {
                    bool is_valid = test_coloring_graph(graph);
                    if (!is_valid) {
                        if (sVerbose) std::cout << "  [xCL] " << graph << std::endl;
                    }
                    return !is_valid;
                }), mColoringGraphs.end());
}

// Old individual approach removed - now using comprehensive find_all_coloring_graphs_for_value

void Analyzer::find_coloring_graphs() {
    // Clear existing graphs since we'll rebuild them completely
    mColoringGraphs.clear();
    
    // For each value, find all connected components
    for (int value = kOne; value <= kNine; ++value) {
        Value val = static_cast<Value>(value);
        find_all_coloring_graphs_for_value(val);
    }
}

void Analyzer::find_all_coloring_graphs_for_value(const Value &value) {
    std::unordered_set<Coord> visited_global;
    
    // Find all cells with this candidate value
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            Coord coord(row, col);
            const Cell &cell = mBoard->at(coord);
            
            if (!cell.isNote() || !cell.check(value)) continue;
            if (visited_global.find(coord) != visited_global.end()) continue;
            
            // Start a new connected component from this cell
            ColoringGraph new_graph;
            new_graph.value = value;
            
            std::queue<std::pair<Coord, ColoringColor>> to_process;
            std::unordered_set<Coord> visited_local;
            
            to_process.push({coord, kColorA});
            new_graph.cells[coord] = kColorA;
            visited_local.insert(coord);
            visited_global.insert(coord);
            
            while (!to_process.empty()) {
                auto [current_coord, current_color] = to_process.front();
                to_process.pop();
                
                const Cell &current_cell = mBoard->at(current_coord);
                
                // Find all cells strongly linked to this cell
                std::vector<Cell> linked_cells;
                
                // Check row for strong link
                auto row_candidates = find_strong_link_candidates(mBoard->row(current_cell), value);
                if (row_candidates.size() == 2 && (row_candidates[0] == current_cell || row_candidates[1] == current_cell)) {
                    Cell other_cell = (row_candidates[0] == current_cell) ? row_candidates[1] : row_candidates[0];
                    linked_cells.push_back(other_cell);
                }
                
                // Check column for strong link
                auto col_candidates = find_strong_link_candidates(mBoard->column(current_cell), value);
                if (col_candidates.size() == 2 && (col_candidates[0] == current_cell || col_candidates[1] == current_cell)) {
                    Cell other_cell = (col_candidates[0] == current_cell) ? col_candidates[1] : col_candidates[0];
                    linked_cells.push_back(other_cell);
                }
                
                // Check nonet for strong link
                auto nonet_candidates = find_strong_link_candidates(mBoard->nonet(current_cell), value);
                if (nonet_candidates.size() == 2 && (nonet_candidates[0] == current_cell || nonet_candidates[1] == current_cell)) {
                    Cell other_cell = (nonet_candidates[0] == current_cell) ? nonet_candidates[1] : nonet_candidates[0];
                    linked_cells.push_back(other_cell);
                }
                
                // Add linked cells to the graph with appropriate colors
                for (const Cell &linked_cell : linked_cells) {
                    Coord linked_coord = linked_cell.coord();
                    
                    if (visited_local.find(linked_coord) == visited_local.end()) {
                        // New cell - add with opposite color and queue for processing
                        ColoringColor opposite_color = (current_color == kColorA) ? kColorB : kColorA;
                        new_graph.cells[linked_coord] = opposite_color;
                        to_process.push({linked_coord, opposite_color});
                        visited_local.insert(linked_coord);
                        visited_global.insert(linked_coord);
                    } else {
                        // Cell already in graph - verify color consistency
                        ColoringColor expected_color = (current_color == kColorA) ? kColorB : kColorA;
                        if (new_graph.get_color(linked_coord) != expected_color) {
                            // Color conflict detected - this means we found a cycle with odd length
                            // which shouldn't happen in a valid coloring graph
                            if (sVerbose) {
                                std::cout << "  [!CL] Color conflict detected at " << linked_coord 
                                          << " for value " << value << " - invalid coloring graph" << std::endl;
                            }
                            // Skip this graph as it's invalid
                            new_graph.cells.clear();
                            break;
                        }
                    }
                }
                
                // If graph was invalidated due to color conflict, break out of main loop too
                if (new_graph.cells.empty()) break;
            }
            
            // Only add valid graphs with at least 2 cells
            if (new_graph.size() >= 2) {
                mColoringGraphs.push_back(new_graph);
                if (sVerbose) std::cout << "  [fCL] " << new_graph << std::endl;
            }
        }
    }
}

bool Analyzer::act_on_coloring_graph() {
    if (mColoringGraphs.empty()) {
        return false;
    }
    
    bool acted = false;
    
    // Group all coloring graphs by value
    std::unordered_map<Value, std::vector<ColoringGraph*>> graphs_by_value;
    for (auto &graph : mColoringGraphs) {
        graphs_by_value[graph.value].push_back(&graph);
    }
    
    // Process each value's graphs together
    for (auto &[value, graphs] : graphs_by_value) {
        if (graphs.empty()) continue;
        
        // First, check rule 1 (same color same unit) within each individual graph
        for (auto graph_ptr : graphs) {
            const auto &graph = *graph_ptr;
            
            if (sVerbose) std::cout << "  Processing coloring graph: " << graph << std::endl;
            
            // Group cells by color within this graph
            std::vector<Coord> color_a_cells, color_b_cells;
            for (const auto &[coord, color] : graph) {
                if (color == kColorA) {
                    color_a_cells.push_back(coord);
                } else {
                    color_b_cells.push_back(coord);
                }
            }
            
            // Check rule 1: cells of same color in the same unit
            auto check_same_color_same_unit = [this, &graph, &acted](const std::vector<Coord> &coords, const std::string &color_name) {
                for (size_t i = 0; i < coords.size(); ++i) {
                    for (size_t j = i + 1; j < coords.size(); ++j) {
                        const Cell &cell1 = mBoard->at(coords[i]);
                        const Cell &cell2 = mBoard->at(coords[j]);
                        
                        // Check if they're in the same unit
                        bool same_unit = (mBoard->row(cell1) == mBoard->row(cell2)) ||
                                        (mBoard->column(cell1) == mBoard->column(cell2)) ||
                                        (mBoard->nonet(cell1) == mBoard->nonet(cell2));
                        
                        if (same_unit) {
                            // Conflict found! This color cannot be true, so eliminate all cells of this color
                            std::string unit_type = "";
                            if (mBoard->row(cell1) == mBoard->row(cell2)) unit_type = "row";
                            else if (mBoard->column(cell1) == mBoard->column(cell2)) unit_type = "col";
                            else if (mBoard->nonet(cell1) == mBoard->nonet(cell2)) unit_type = "box";
                            
                            for (const Coord &coord : coords) {
                                const Cell &cell = mBoard->at(coord);
                                if (cell.isNote() && cell.check(graph.value)) {
                                    std::cout << "[CL] " << coord << " x" << graph.value << " [" << color_name << " conflict in " << unit_type << "]" << std::endl;
                                    mBoard->clear_note_at(coord, graph.value);
                                    acted = true;
                                }
                            }
                            return true;
                        }
                    }
                }
                return false;
            };
            
            if (check_same_color_same_unit(color_a_cells, "A") || 
                check_same_color_same_unit(color_b_cells, "B")) {
                // If we acted on this graph due to same-color conflict, continue to next graph
                continue;
            }
        }
        
        // Now check rule 2: cells that can see colors from different graphs
        // Collect all colored cells across all graphs for this value
        std::vector<std::pair<Coord, ColoringColor>> all_colored_cells;
        for (const auto graph_ptr : graphs) {
            for (const auto &[coord, color] : *graph_ptr) {
                all_colored_cells.push_back({coord, color});
            }
        }
        
        // Check each cell to see if it can see cells of both colors
        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                Coord coord(row, col);
                const Cell &cell = mBoard->at(coord);
                
                if (!cell.isNote() || !cell.check(value)) continue;
                
                // Skip if this cell is part of any coloring graph
                bool is_in_graph = false;
                for (const auto graph_ptr : graphs) {
                    if (graph_ptr->contains(coord)) {
                        is_in_graph = true;
                        break;
                    }
                }
                if (is_in_graph) continue;
                
                bool sees_color_a = false, sees_color_b = false;
                
                // Check if this cell can see cells of both colors across all graphs
                for (const auto &[colored_coord, color] : all_colored_cells) {
                    const Cell &colored_cell = mBoard->at(colored_coord);
                    if ((mBoard->row(cell) == mBoard->row(colored_cell)) ||
                        (mBoard->column(cell) == mBoard->column(colored_cell)) ||
                        (mBoard->nonet(cell) == mBoard->nonet(colored_cell))) {
                        
                        if (color == kColorA) {
                            sees_color_a = true;
                            if (sVerbose) std::cout << "    " << coord << " sees color A at " << colored_coord << std::endl;
                        } else {
                            sees_color_b = true;
                            if (sVerbose) std::cout << "    " << coord << " sees color B at " << colored_coord << std::endl;
                        }
                    }
                }
                
                if (sees_color_a && sees_color_b) {
                    std::cout << "[CL] " << coord << " x" << value << " [sees both colors]" << std::endl;
                    mBoard->clear_note_at(coord, value);
                    acted = true;
                }
            }
        }
    }
    
    // Remove one graph that we processed (to maintain the original behavior of processing one at a time)
    if (!mColoringGraphs.empty()) {
        mColoringGraphs.pop_back();
    }
    
    return acted;
}

std::ostream& operator<<(std::ostream& outs, const Analyzer::ColoringGraph &graph) {
    outs << "{";
    bool first = true;
    for (const auto &[coord, color] : graph) {
        if (!first) outs << ",";
        first = false;
        outs << coord << ":" << (color == Analyzer::kColorA ? "A" : "B");
    }
    outs << "}#" << graph.value;
    return outs;
}
