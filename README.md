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
```

There are currently four (TODO: five) implemented heuristics: `naked-single`, denoted as `[NS]`, `hidden-single`, denoted as `[HS]`, `locked-candidates`, denoted as `[LC]`, and `naked-pairs`, denoted as `[NP]`.

For each heuristic, the number of available actions associated with the heuristic appears in parentheses, followed by a summary description of such actions.

Naked single indicates a note cell where only one possible value remains. The coordinates of all such cells are recorded. Going back to the example above:
```
[NS](2) {[8, 5], [7, 8]}
```
This is interpreted as saying there are two naked singles on the board, at row 8, column 5 and at row 7, column 8.
