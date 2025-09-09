# Sudoku Solver Usage

## Running the solver
To run `./sudoku-solver`:
- Takes input from stdin
- Enable verbose output first with: `v`
- Load new board with: `n.` followed by 81 characters of board description
- Use dots (.) for empty cells

Example:
```bash
echo -e "v\nn.1.....569492.561.8.561.924...964.8.1.64.1....218.356.4.4.5...169.5.614.2621.....5" | ./sudoku-solver
```

## Step-by-step execution
- Use `.` command to execute one solving step at a time
- Multiple steps: `\n.\n.\n.\n.` etc.
- To see XWing eliminations in action:
```bash
echo -e "v\nn.board_string\n.\n.\n.\n.\n.\n.\n.\n.\n.\n." | ./sudoku-solver | grep -E "\[XW\].*x"
```

## Pattern abbreviations in output
- [NS] = Naked Singles
- [HS] = Hidden Singles  
- [NP] = Naked Pairs
- [LC] = Locked Candidates
- [HP] = Hidden Pairs
- [XW] = XWing patterns

## Reading board display output
The visual board display shows each Sudoku cell as a 3x3 grid of positions representing candidate values:

```
Position mapping within each cell:
1 2 3
4 5 6
7 8 9
```

- Each `*` symbol represents a candidate value at its corresponding position
- For example, `*` in top-right position = candidate 3, middle-left = candidate 4, etc.
- Solved cells show the actual digit instead of candidates
- Each logical board row requires reading **three display lines** (ignore horizontal separator lines)
- Example cell with candidates 1,3,7: `[*   *][ ][*    ]`

### Reading methodology:
1. **Systematic approach**: Go through each logical row (1-9) systematically
2. **Three-line groups**: Each logical row spans exactly 3 display lines
3. **Cell-by-cell**: Within each row, examine each column position (1-9) 
4. **Position accuracy**: Look precisely at the 3x3 grid position for the target candidate
5. **Distinguish solved vs notes**: Solved cells show digits, note cells show `*` patterns
6. **Double-check**: When analyzing patterns like XWing, verify ALL cells in the relevant rows/columns

### XWing pattern validation:
- **Row constraint**: Two specific rows must have exactly 2 candidates each for the target value
- **Column elimination**: The corresponding columns will have 2+ candidates for the target value
- **Rectangle formation**: All four corner positions must contain the candidate
- **Valid elimination**: Remove candidates from columns (except the two constraint rows)