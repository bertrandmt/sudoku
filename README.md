# sudoku-solver
`sudoku-solver` is a solving tool for Sudoku.

# Usage

```
$ ./sudoku-solver
λ 
```

At the prompt (`λ`), start by creating a new board with the `n` command.

There are two forms for the command.

## Form 1
Form 1 is denoted by a `.` immediately following the `n` command. In this form, all 81 cells in the board are entered in one long series, going left to right, from top row to bottom row. Value cells are entered as their digit value. Unset cells are entered as a `.`. White space is okay and ignored.

For example:
```
λ n.7.415...3 ......17. .526..... ..62...57 ......... .......32 ..3.4...6 615.3..2. 4.7....9.
[...]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |  *  ][  *  |     |     ]
[  7  |    *|  4  ][  1  |  5  |     ][    *|    *|  3  ]
[     |  * *|     ][     |     |  * *][  * *|  *  |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[    *|    *|     ][    *|  *  |  * *][     |     |     ]
[     |    *|     ][*    |     |*    ][  1  |  7  |* *  ]
[  * *|  * *|  * *][  * *|  * *|  * *][     |     |  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|     |     ][     |     |    *][     |     |     ]
[     |  5  |  2  ][  6  |     |*    ][*    |*    |*    ]
[  * *|     |     ][     |* * *|* * *][  * *|  *  |  * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[*   *|    *|     ][     |*    |*   *][     |     |     ]
[     |*    |  6  ][  2  |     |*    ][*    |  5  |  7  ]
[  * *|  * *|     ][     |  * *|  * *][  * *|     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* * *|  * *|*    ][    *|*    |*   *][     |*    |*    ]
[  *  |*    |     ][* *  |    *|* * *][*   *|*   *|*    ]
[  * *|* * *|  * *][* * *|* * *|* * *][  * *|  *  |  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*    |     |*    ][     |*    |*    ][     |     |     ]
[  *  |*    |     ][* *  |    *|* * *][*   *|  3  |  2  ]
[  * *|* * *|  * *][* * *|* * *|* * *][  * *|     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[  *  |  *  |     ][     |     |* *  ][     |*    |     ]
[     |     |  3  ][  *  |  4  |  *  ][  *  |     |  6  ]
[  * *|  * *|     ][* * *|     |* * *][* *  |  *  |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  6  |  1  |  5  ][     |  3  |     ][*    |  2  |*    ]
[     |     |     ][* * *|     |* * *][* *  |     |  *  ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |  *  |     ][     |* *  |* *  ][    *|     |*    ]
[  4  |     |  7  ][  *  |    *|  * *][  *  |  9  |  *  ]
[     |  *  |     ][  *  |  *  |  *  ][  *  |     |  *  ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[...]
λ 
```

The table, as entered, is loaded, analyzed and displayed.

## Form 2
Form 2 is denoted by a ';' immediately following the `n` command. In this form, only set (value) cells are entered, as a series of `rcv` entries, with `r` denoting the row, `c` denoting the column and `v` denoting the value, each as single digits. Entries are separated by `;` characters. White space is okay and ignored.

For example:
```
λ n;116;127;174;181;235;354;387;396;446;464;485;532;541;573;655;682;738;756;765;817;842;868;883;944;957;963
[...]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |    *][    *|  * *|  *  ][     |     |  * *]
[  6  |  7  |     ][  *  |     |     ][  4  |  1  |  *  ]
[     |     |    *][  * *|  * *|    *][     |     |  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* * *|* * *|     ][    *|* * *|* *  ][  *  |     |  * *]
[*    |*    |  5  ][     |     |    *][     |     |     ]
[  * *|  * *|     ][* * *|  * *|*   *][  * *|  * *|  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* * *|* * *|*   *][    *|     |* *  ][  *  |     |     ]
[     |     |     ][  *  |  4  |     ][  *  |  7  |  6  ]
[  * *|  * *|    *][  * *|     |    *][  * *|     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[*   *|*   *|*   *][     |  * *|     ][*    |     |*    ]
[     |     |     ][  6  |     |  4  ][     |  5  |     ]
[  * *|  * *|*   *][     |  * *|     ][* * *|     |* * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[* *  |* * *|  2  ][  1  |     |     ][  3  |*   *|*    ]
[  * *|  * *|     ][     |  * *|*   *][     |  * *|* * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|*   *|*   *][    *|     |     ][*    |     |*    ]
[*    |*   *|*   *][     |  5  |     ][    *|  2  |*    ]
[  * *|  * *|*   *][* * *|     |*   *][* * *|     |* * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[* * *|* * *|     ][     |     |     ][* *  |     |* *  ]
[*    |*    |  8  ][     |  6  |  5  ][     |*    |*    ]
[    *|    *|     ][    *|     |     ][*   *|    *|*   *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |*    |*    ][     |*    |     ][*    |     |*    ]
[  7  |* * *|*   *][  2  |     |  8  ][  * *|  3  |* *  ]
[     |    *|    *][     |    *|     ][    *|     |    *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* *  |* *  |*    ][     |     |     ][* *  |     |* *  ]
[  *  |  * *|    *][  4  |  7  |  3  ][  * *|    *|  *  ]
[    *|    *|    *][     |     |     ][  * *|  * *|  * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[...]
λ
```

Just as in form 1, the table, as entered, is loaded, analyzed and displayed.

# Solver

Once a game is loaded, the solver can be made use of. It is stateful and at each state carries a "full" (as full as possible) analysis of the state of the game.

Four commands impact the state of the board:

* `>` or `.` proceeds through one action in advancing the state of resolution.
* `<` or `,` goes back one step
* `r` runs as many actions as possible until full resolution or no remaining heuristic remains
* `!` resets the state of the board to its initial state (as entered with the `n` command)

For each state, including the initial state from a new game, the full set of possible next non-obvious moves (outside of row, column, nonet filtering of notes) is recorded after printing the board:

```
λ >
Step #1:
[NS] cell[7, 4] =9
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |    *][    *|  * *|  *  ][     |     |  * *]
[  6  |  7  |     ][  *  |     |     ][  4  |  1  |  *  ]
[     |     |    *][  *  |  * *|    *][     |     |  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* * *|* * *|     ][    *|* * *|* *  ][  *  |     |  * *]
[*    |*    |  5  ][     |     |    *][     |     |     ]
[  * *|  * *|     ][* *  |  * *|*   *][  * *|  * *|  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* * *|* * *|*   *][    *|     |* *  ][  *  |     |     ]
[     |     |     ][  *  |  4  |     ][  *  |  7  |  6  ]
[  * *|  * *|    *][  *  |     |    *][  * *|     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[*   *|*   *|*   *][     |  * *|     ][*    |     |*    ]
[     |     |     ][  6  |     |  4  ][     |  5  |     ]
[  * *|  * *|*   *][     |  * *|     ][* * *|     |* * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[* *  |* * *|  2  ][  1  |     |     ][  3  |*   *|*    ]
[  * *|  * *|     ][     |  * *|*   *][     |  * *|* * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|*   *|*   *][    *|     |     ][*    |     |*    ]
[*    |*   *|*   *][     |  5  |     ][    *|  2  |*    ]
[  * *|  * *|*   *][* *  |     |*   *][* * *|     |* * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[* * *|* * *|     ][     |     |     ][* *  |     |* *  ]
[*    |*    |  8  ][  9  |  6  |  5  ][     |*    |*    ]
[     |     |     ][     |     |     ][*    |     |*    ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |*    |*    ][     |*    |     ][*    |     |*    ]
[  7  |* * *|*   *][  2  |     |  8  ][  * *|  3  |* *  ]
[     |    *|    *][     |     |     ][    *|     |    *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[* *  |* *  |*    ][     |     |     ][* *  |     |* *  ]
[  *  |  * *|    *][  4  |  7  |  3  ][  * *|    *|  *  ]
[    *|    *|    *][     |     |     ][  * *|  * *|  * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[NS](2) {[8, 5], [7, 8]}
[HS](2) {[2, 6]#6[n], [4, 5]#2[n]}
[LC](5) {{{[1, 6],[2, 6],[3, 6]}#2[^n]}, {{[2, 6],[3, 6]}#1[^n]}, {{[4, 5]}#2[^c]}, {{[8, 5]}#1[^r]}, {{[8, 5]}#1[^c]}}
[NP](1) {{{[5, 6],[6, 6]}#{7,9}}}
[HP](0)
```

There are currently five implemented heuristics: `naked-single`, denoted as `[NS]`, `hidden-single`, denoted as `[HS]`, `locked-candidates`, denoted as `[LC]`, `naked-pairs`, denoted as `[NP]`, and `hidden-pairs`, denoted as `[HP]`.

For each heuristic, the number of available actions associated with the heuristic appears in parentheses, followed by a summary description of such actions.

Naked single indicates a note cell where only one possible value remains. The coordinates of all such cells are recorded. Going back to the example above:
```
[NS](2) {[8, 5], [7, 8]}
```
This is to be interpreted as saying there are two naked singles on the board, at row 8, column 5 and at row 7, column 8.

A hidden single is a note cell where one of the possible values is the only possible value in its row, column or nonet. For each possible hidden single, the coordinates of the hidden single cell, the corresponding value and the set (row, column or nonet) where that value is single are all recorded. In the example above:
```
[HS](2) {[2, 6]#6[n], [4, 5]#2[n]}
```
This indicates there are two hidden singles in the board. The cell at row 2, column 6 is the only possibiliy for value 6 in its nonet. The cell at row 4, column 5 is the only possible cell for value 2 in its nonet.

Borrowing from the Sudoku Assistant [explainer page](https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks), the locked candidate rule states that when a given candidate is possible at the intersection of a nonet and a row (or column), and that same candidate is not possible in the rest of the same nonet, then it is not possible in the rest of the same row/column either. The same goes when the candidate is not possible in the rest of the same row/column, in which case it will not be possible in the rest of the same nonet. In the example above:
```
[LC](5) {{{[1, 6],[2, 6],[3, 6]}#2[^n]}, {{[2, 6],[3, 6]}#1[^n]}, {{[4, 5]}#2[^c]}, {{[8, 5]}#1[^r]}, {{[8, 5]}#1[^c]}}
```
This indicates there are five possible locked candidates in the board. The first entry is:
```
{{[1, 6],[2, 6],[3, 6]}#2[^n]}
```
This indicates that cells at row 1, column 6, and row 2, column 6 and row 3, column 6 are all candidates for value 2 and that no other cell in their nonet can be a candidate for value 2 (since, though this is not explicitly stated, but can be readily verified in the board above, no other cell in their column is a candidate for value 2). The action for this particular locked candidates finding would therefore be to remove 2 as candidate for cells 1,5 and 2,5 (with celll 3,5 being a value cell, so not impacted, and none of the cells in column 4 for this nonet being candidates for value 2).

Naked pairs are pairs of note cells with the same two (and only two) possible candidate values, in the same set (row, column or nonet). In the example above:
```
[NP](1) {{{[5, 6],[6, 6]}#{7,9}}}
```
This indicates there is one naked pair in the board, in cells at row 5, column 6 and row 6, column 6. Their common pair of possible values is 7 and 9. The action taken if acting on this entry would be to remove 7 and 9 as candidates in the cells in their common column and common nonet. In practice, this translates in removing candidate 9 in cell 1,6, candidates 7 and 9 in cell 2,6, candidate 9 in cell 3,6, candidate 9 in cells 4,5 and 5,5 and candidate 7 in cell 6,4.

Hidden pairs are pairs of note cells with two common candidate values (among others). They are alone in their set (row, column or nonet) sharing these two values. The syntax is the same as that of the naked pair, that is: a pair of coordinates and the two common values.

The solver will process naked singles as long as there are some available, followed by hidden singles, followed by locked candidates, followed by naked pairs, finally followed by hidden pairs.

# Other commands

The command `p` prints the current state of the board in `.` notation (see [Form 1](#form-1) above).

The command `v` toggles verbosity of the analysis of the board state. By default, analysis is *not* verbose.
