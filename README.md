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

Once a game is loaded, the solver can be made use of. It is stateful and at each state carries a partial analysis of the state of the game, stopping analysis when a heuristic finds actionable step(s).

Five commands impact the state of the board:

* `>` or `.` proceeds through one action in advancing the state of resolution.
* `<` or `,` goes back one step
* `r` runs as many actions as possible until full resolution or no remaining heuristic remains
* `s` runs as many "simple" actions as possible. Simple actions include 'naked' and 'singles' heuristics
* `!` resets the state of the board to its initial state (as entered with the `n` command)

Notes are updated (filtered out) on loading and after each action that sets a value.

For each state, including the initial state from a new game, the full set of possible moves for the first heuristic that has such moves is recorded after printing the board:

```
λ >
Step #1:
[NS] [7, 4] =9
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
Left to solve:   54
Notes remaining: 210
[NS](2) {[7, 8]#4, [8, 5]#1}
[HS](0) {}
[NP](0) {}
[LC](0) {}
[HP](0) {}
[XW](0) {}
[SC](0) {}
[YW](0) {}
[SF](0) {}
[XY](0) {}
```

There are currently ten implemented heuristics: 

1. `naked-single`, denoted as `[NS]`,
1. `hidden-single`, denoted as `[HS]`,
1. `naked-pairs`, denoted as `[NP]`,
1. `locked-candidates`, denoted as `[LC]`,
1. `hidden-pairs`, denoted as `[HP]`,
1. `x-wing`, denoted as `[XW]`,
1. `simple-coloring`, denoted as `[SC]`,
1. `y-wing`, denoted as `[YW]`,
1. `swordfish`, denoted as `[SF]`, and
1. `XY-chain`, denoted as `[XY]`.

For each heuristic, the number of available actions associated with the heuristic appears in parentheses, followed by a summary description of such actions.

## Naked Singles

Naked single indicates a note cell where only one possible value remains. The coordinates of all such cells are recorded. Going back to the example above:
```
[NS](2) {[7, 8]#4, [8, 5]#1}
```
This is to be interpreted as saying there are two naked singles on the board, at row 7, column 8 for value 4 and at row 8, column 5 for value 1.

## Hidden Singles

A hidden single is a note cell where one of the possible values is the only possible value in its row, column or nonet. For each possible hidden single, the coordinates of the hidden single cell, the corresponding value and the set (row, column or nonet) where that value is single are all recorded. For example:
```
[HS](2) {[2, 6]#6[r], [4, 5]#2[r]}
```
This indicates there are two hidden singles in the board. The cell at row 2, column 6 is the only possibiliy for value 6 in its row. The cell at row 4, column 5 is the only possible cell for value 2 in its row.

## Naked Pairs

Naked pairs are pairs of note cells with the same two (and only two) possible candidate values, in the same set (row, column or nonet). In the example above:
```
[NP](1) {{{[5, 6],[6, 6]}#{7,9}}}
```
This indicates there is one naked pair in the board, in cells at row 5, column 6 and row 6, column 6. Their common pair of possible values is 7 and 9. The action taken if acting on this entry would be to remove 7 and 9 as candidates in the cells in their common column and common nonet. In practice, this translates in removing candidate 9 in cell 1,6, candidates 7 and 9 in cell 2,6, candidate 9 in cell 3,6, candidate 9 in cells 4,5 and 5,5 and candidate 7 in cell 6,4.

## Locked Candidates

Borrowing from the Sudoku Assistant [explainer page](https://www.stolaf.edu/people/hansonr/sudoku/explain.htm#blocks), the locked candidate rule states that when a given candidate is possible at the intersection of a nonet and a row (or column), and that same candidate is not possible in the rest of the same nonet, then it is not possible in the rest of the same row/column either. The same goes when the candidate is not possible in the rest of the same row/column, in which case it will not be possible in the rest of the same nonet. In the example above:
```
[LC](5) {{{[1, 6],[2, 6],[3, 6]}#2[^n]}, {{[2, 6],[3, 6]}#1[^n]}, {{[4, 5]}#2[^c]}, {{[8, 5]}#1[^r]}, {{[8, 5]}#1[^c]}}
```
This indicates there are five possible locked candidates in the board. The first entry is:
```
{{[1, 6],[2, 6],[3, 6]}#2[^n]}
```
This indicates that cells at row 1, column 6, and row 2, column 6 and row 3, column 6 are all candidates for value 2 and that no other cell in their nonet can be a candidate for value 2 (since, though this is not explicitly stated, but can be readily verified in the board above, no other cell in their column is a candidate for value 2). The action for this particular locked candidates finding would therefore be to remove 2 as candidate for cells 1,5 and 2,5 (with celll 3,5 being a value cell, so not impacted, and none of the cells in column 4 for this nonet being candidates for value 2).

## Hidden Pairs

Hidden pairs are pairs of note cells with two common candidate values (among others). They are alone in their set (row, column or nonet) sharing these two values. The syntax is the same as that of the naked pair, that is: a pair of coordinates and the two common values.

For example:
```
[HP](2) {{{[3, 1],[9, 1]}#{4,6}}, {{[6, 6],[9, 6]}#{1,7}}}
```

This indicates there are two hidden pairs at that point in the resolution of a particular board. The first hidden pair is cells at row 3, column 1 and row 9, column 1 (on the same column), for candidate values 4 and 6.

## X-Wing

The Sudoku Wiki [explainer page](https://www.sudokuwiki.org/x_wing_strategy) on the X-Wing Strategy states: "When there are only two possible cells for a value in each of two different rows, and these candidates lie also in the same columns, then all other candidates for this value in the columns can be eliminated." The same is true when swapping rows and columns. There is an additional dimension when considering nonets instead of rows/columns, but that dimension does not usually fall under the X-Wing Strategy moniker.

As an example:
```
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[*   *|  * *|* * *][    *|     |    *][     |     |     ]
[  *  |  *  |  *  ][  * *|    *|  * *][    *|  9  |  4  ]
[  *  |     |  *  ][  *  |* *  |* *  ][*    |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][  * *|     |  * *]
[  7  |  6  |*    ][  9  |  1  |*    ][     |  5  |     ]
[     |     |  *  ][     |     |  *  ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[    *|     |    *][    *|     |     ][     |     |     ]
[* *  |  9  |* *  ][* * *|*   *|  2  ][    *|  8  |  1  ]
[     |     |     ][     |*    |     ][*    |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[    *|     |  * *][  *  |     |     ][  * *|     |  * *]
[*   *|  7  |*   *][*   *|  5  |*   *][*    |  1  |     ]
[     |     |    *][  *  |     |  *  ][  * *|     |  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|  * *|* * *][     |  *  |     ][  * *|  * *|  * *]
[* * *|  *  |* * *][  7  |*   *|  9  ][* *  |     |     ]
[     |     |     ][     |  *  |     ][  *  |     |  *  ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |  *  ][  *  |     |     ][  *  |     |     ]
[* *  |  8  |* *  ][*    |  3  |  1  ][* *  |  6  |  7  ]
[     |     |    *][     |     |     ][    *|     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |    *][     |     |    *][    *|     |    *]
[  2  |  4  |  * *][  1  |    *|  * *][     |  7  |    *]
[     |     |  *  ][     |  *  |  *  ][  * *|     |  * *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[    *|     |    *][  * *|     |    *][  * *|     |     ]
[    *|  1  |    *][    *|  9  |    *][     |  4  |  5  ]
[  *  |     |* *  ][  *  |     |* *  ][  *  |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |    *|    *][  * *|  *  |    *][     |  * *|  * *]
[  9  |  *  |  * *][* * *|*   *|* * *][  1  |     |    *]
[     |     |* *  ][  *  |* *  |* *  ][     |     |  *  ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
Left to solve:   50
Notes remaining: 178
[NS](0) {}
[HS](0) {}
[NP](0) {}
[LC](0) {}
[HP](0) {}
[XW](1) {{{[5, 5],[9, 8]}#2[^r]}}
[SC](0) {}
[YW](0) {}
[XY](0) {}
```
This indicates there is one X-Wing structure in the board, with top-left and bottom-right corners as cells at row 5, column 5 and row 9, column 8 respectively, for candidate value 2.

As can be observed, cells at `[5, 5]` and `[9, 5]` are candidates for value 2, and no other cell in column 5 is. Similarly, cells at `[5, 8]` and `[9, 8]` are candidates for value 2 and no other cell in column 8 is. Additionally, cells at `[5, 2]`, `[5, 3]`, `[5, 7]`, `[5, 9]`, `[9, 4]` and `[9, 9]` are all additional candidates for value 2 and, per the heuristic, can be eliminated.

When executing on this heuristic, the solver takes the following set of actions:
```
λ >
Step #3:
[XW] [5, 2] x2 [r]
[XW] [5, 3] x2 [r]
[XW] [5, 7] x2 [r]
[XW] [5, 9] x2 [r]
[XW] [9, 4] x2 [r]
[XW] [9, 9] x2 [r]
```

## Simple Coloring

Per the Sudoku Wiki's [explainer page](https://www.sudokuwiki.org/Simple_Colouring) on Simple Coloring, this is a chaining strategy. For a given candidate value, we are building a graph of candidate cells for this value, linked by 'bi-location' links, and sporting alternate 'green' and 'red' colors.

A 'bi-location' link is a link between a candidate for a given value and another candidate for the same value in the same row, column or nonet, *if* there are no additional candidate for the same value in the same row, column or nonet.

The resulting graph is a "color chain".

Action is by applying two rules:
* Rule 2 - for a given color chain, if any row, column or nonet has the same color twice, all candidates which share that color in the chain can be eliminated.
* Rule 4 - for a given color chain, if a candidate for the value that it *not* on the chain can see two colors on the chain, then it can be eliminated.

For example:
```
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][*    |     |*    ][     |     |     ]
[  2  |  8  |  9  ][*   *|*   *|*    ][  3  |  7  |  5  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  3  |  6  |  4  ][  *  |  9  |  *  ][  8  |  1  |  2  ]
[     |     |     ][*    |     |*    ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  5  |  1  |  7  ][  2  |  8  |  3  ][  9  |  6  |  4  ]
[     |     |     ][     |     |     ][     |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |     ][     |     |     ]
[  8  |  9  |  3  ][* *  |  2  |* *  ][  6  |* *  |  1  ]
[     |     |     ][*    |     |*    ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  1  |  4  |  5  ][  8  |  3  |  6  ][  7  |  2  |  9  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][*    |     |*    ][     |     |     ]
[  7  |  2  |  6  ][     |* *  |     ][* *  |  8  |  3  ]
[     |     |     ][    *|     |    *][     |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |     ][     |     |     ]
[  4  |  5  |  1  ][  3  |  7  |  8  ][  2  |  9  |  6  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[    *|  7  |  2  ][* * *|  1  |* *  ][* *  |  3  |  8  ]
[    *|     |     ][    *|     |    *][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[    *|  3  |  8  ][* * *|* * *|  2  ][  1  |* *  |  7  ]
[    *|     |     ][    *|     |     ][     |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
Left to solve:   20
Notes remaining: 49
[NS](0) {}
[HS](0) {}
[NP](0) {}
[LC](0) {}
[HP](0) {}
[XW](0) {}
[SC](1) {{{[4, 8]🟩,[9, 8]🟥,[8, 7]🟩,[6, 5]🟩,[6, 7]🟥}#4}}
[YW](0) {}
[XY](0) {}
```
This indicates there is a simple color chain for candidate value 4, running via `[4, 8]`, `[9, 8]`, `[8, 7]`, `[6, 7]` and `[6, 5]`.

Per "Rule 4", `[9, 5]` sees both red `[9, 8]` and green `[6, 5]` and therefore cannot be a candidate for value 4:
```
[SC] [9, 5] x4 [👀🟩🟥]
```
Later, a different chain for candidate value 5 is found, running through `[4, 8]`, `[9, 8]`, `[8, 7]`, `[6, 7]`, `[6, 5]` and `[9, 5]`:
```
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][*    |     |*    ][     |     |     ]
[  2  |  8  |  9  ][    *|*   *|*    ][  3  |  7  |  5  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  3  |  6  |  4  ][  *  |  9  |  *  ][  8  |  1  |  2  ]
[     |     |     ][*    |     |*    ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  5  |  1  |  7  ][  2  |  8  |  3  ][  9  |  6  |  4  ]
[     |     |     ][     |     |     ][     |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |     ][     |     |     ]
[  8  |  9  |  3  ][* *  |  2  |* *  ][  6  |* *  |  1  ]
[     |     |     ][*    |     |*    ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[  1  |  4  |  5  ][  8  |  3  |  6  ][  7  |  2  |  9  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][*    |     |*    ][     |     |     ]
[  7  |  2  |  6  ][     |* *  |     ][* *  |  8  |  3  ]
[     |     |     ][    *|     |    *][     |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |     ][     |     |     ]
[  4  |  5  |  1  ][  3  |  7  |  8  ][  2  |  9  |  6  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[    *|  7  |  2  ][* * *|  1  |* *  ][* *  |  3  |  8  ]
[    *|     |     ][    *|     |    *][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |     ][     |     |     ][     |     |     ]
[    *|  3  |  8  ][* * *|  * *|  2  ][  1  |* *  |  7  ]
[    *|     |     ][    *|     |     ][     |     |     ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
Left to solve:   20
Notes remaining: 47
[NS](0) {}
[HS](0) {}
[NP](0) {}
[LC](0) {}
[HP](0) {}
[XW](0) {}
[SC](1) {{{[4, 8]🟩,[9, 8]🟥,[8, 7]🟩,[6, 5]🟩,[6, 7]🟥,[9, 5]🟥}#5}}
[YW](0) {}
[SF](0) {}
[XY](0) {}
```
However, `[9, 5]` is colored red, but sees `[9, 8]` also colored red. Per "Rule 2", all reds from the chain can be eliminated:
```
[SC] [9, 5] x5 [r🟥]
[SC] [6, 7] x5 [r🟥]
[SC] [9, 8] x5 [r🟥]
```
Additionally, "Rule 4" also applies, with `[8, 4]` and `[8, 6]` seeing both red at `[9, 5]` (same nonet) and green at `[8, 7]` (same row):
```
[SC] [8, 4] x5 [👀🟩🟥]
[SC] [8, 6] x5 [👀🟩🟥]
```

## Y-Wing

TODO

## Swordfish

TODO

## XY-Chain

TODO

## Order of analysis and resolution

The solver will stop analysis when a heuristic detects possible actions. The order of evaluation of heuristics is as in the ordered list above. When solving for a given heuristic, in a given step, the solver will execute on *all* possible actions for Naked Singles, Hidden Singles, Haked Pairs, Locked Candidates, Hidden Pairs and Y-Wing, and on only the first possible action for X-Wing, Simple Coloring, Swordfish and XY-Chain.

# Editing the table

It is possible to manually edit the board after initial load. Two commands are available for this purpose:

* `x` removes a candidate in a given note
* `=` sets a given board entry to a value

The syntax for both is `Crcv` where `C` is the command (`x` or `=`), `r` is the row, `c` is the column for the cell to be changed, `v` is the value for the command.

For example, from the state above:
```
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |    *|     ][     |     |    *]
[  6  |*    |*    ][  1  |     |  8  ][  2  |  5  |     ]
[     |    *|    *][     |*    |     ][     |     |*    ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |*   *|*   *][  * *|  * *|    *][     |     |     ]
[  5  |     |     ][    *|     |    *][  9  |  8  |  4  ]
[     |*    |*    ][*    |*    |*    ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |    *|     ][     |     |     ][    *|     |    *]
[  8  |     |  2  ][  *  |  4  |  *  ][    *|  1  |    *]
[     |*    |     ][    *|     |    *][     |     |*    ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |     ][     |     |     ]
[  4  |    *|  *  ][     |  *  |  1  ][    *|  3  |  2  ]
[     |    *|    *][* *  |* *  |     ][  *  |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |*    ][    *|     |    *][     |     |*    ]
[  2  |  8  |  *  ][    *|  9  |    *][  7  |  4  |  *  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |*   *|*   *][     |     |     ][*    |     |*    ]
[  7  |    *|  *  ][  4  |  *  |  2  ][    *|  9  |  * *]
[     |     |     ][     |  *  |     ][  *  |     |  *  ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][    *|     |    *][    *|     |    *]
[  9  |  2  |  6  ][  *  |  1  |* *  ][*    |  7  |     ]
[     |     |     ][  *  |     |     ][  *  |     |  *  ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|     |     ][    *|     |    *][*   *|     |*   *]
[     |  5  |  8  ][     |  6  |*    ][*    |  2  |     ]
[     |     |     ][*   *|     |*   *][     |     |    *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|     |     ][  * *|  * *|    *][     |     |*   *]
[     |*    |*    ][     |     |     ][  5  |  6  |     ]
[     |*    |*    ][  * *|  *  |    *][     |     |  * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[...]
λ x927
  [fNS] [9, 2]
  [fHS] [9, 3]#7[n]
  [fLC] {[2, 2],[3, 2]}#7[^n]
  [fLC] {[9, 3]}#7[^c]
Step #11:
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |    *|     ][     |     |    *]
[  6  |*    |*    ][  1  |     |  8  ][  2  |  5  |     ]
[     |    *|    *][     |*    |     ][     |     |*    ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |*   *|*   *][  * *|  * *|    *][     |     |     ]
[  5  |     |     ][    *|     |    *][  9  |  8  |  4  ]
[     |*    |*    ][*    |*    |*    ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |    *|     ][     |     |     ][    *|     |    *]
[  8  |     |  2  ][  *  |  4  |  *  ][    *|  1  |    *]
[     |*    |     ][    *|     |    *][     |     |*    ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][     |     |     ][     |     |     ]
[  4  |    *|  *  ][     |  *  |  1  ][    *|  3  |  2  ]
[     |    *|    *][* *  |* *  |     ][  *  |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |     |*    ][    *|     |    *][     |     |*    ]
[  2  |  8  |  *  ][    *|  9  |    *][  7  |  4  |  *  ]
[     |     |     ][     |     |     ][     |     |     ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[     |*   *|*   *][     |     |     ][*    |     |*    ]
[  7  |    *|  *  ][  4  |  *  |  2  ][    *|  9  |  * *]
[     |     |     ][     |  *  |     ][  *  |     |  *  ]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[     |     |     ][    *|     |    *][    *|     |    *]
[  9  |  2  |  6  ][  *  |  1  |* *  ][*    |  7  |     ]
[     |     |     ][  *  |     |     ][  *  |     |  *  ]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|     |     ][    *|     |    *][*   *|     |*   *]
[     |  5  |  8  ][     |  6  |*    ][*    |  2  |     ]
[     |     |     ][*   *|     |*   *][     |     |    *]
+-----+-----+-----++-----+-----+-----++-----+-----+-----+
[*   *|     |     ][  * *|  * *|    *][     |     |*   *]
[     |*    |*    ][     |     |     ][  5  |  6  |     ]
[     |     |*    ][  * *|  *  |    *][     |     |  * *]
+=====+=====+=====++=====+=====+=====++=====+=====+=====+
[...]
λ 
```

The `x` command fails and takes no action if the cell at row `r`, column `c` is a value cell, or if value `v` is not a candidate for it.
The `=` command succeeds unconditionally.

The solver will take manual changes into consideration when resuming solving.

It must be noted that manual edition of the board has the potential of rendering it impossible to solve. As an example above, setting the value 7 in the cell at row 9, column 2 quickly leads the autosolver to a condition where a note cell has no candidates. The solver is *not* equipped to handle such cases and will likely fail with an assertion when encountering such a case.
 
# Other commands

The command `p` prints the current state of the board in `.` notation (see [Form 1](#form-1) above).

The command `c` prints the per-cell candidates in a machine-readable form: one logical row per line, each prefixed with a `~` sentinel and followed by nine whitespace-separated fields (one per cell, left to right). A solved cell's field is its digit; a note cell's field is the concatenation of its remaining candidate digits. This is primarily a hook for the test suite (it checks that no solving step ever eliminates a cell's true candidate), but it is also handy for scripting.

The command `v` toggles verbosity of the analysis of the board state. By default, analysis is *not* verbose.

# Building and testing

```sh
make              # optimized build -> ./sudoku-solver
make debug=1      # unoptimized build with debug symbols
make test         # build, then run the black-box correctness suite
make clean        # remove build and coverage artifacts
```

Intermediate artifacts (object files, the auto-generated header-dependency
files, and coverage data) are written under `build/`, so the repo root holds
only sources. The `./sudoku-solver` binary itself is left at the root, where the
tests and the examples above expect it.

The correctness suite lives in `tests/run.sh` and drives the compiled binary
through its REPL. CI additionally builds across gcc, clang and macOS/libc++ and
runs an ASan/UBSan build against adversarial input.

## Coverage

`make coverage=1` produces an instrumented build; running the suite against it
records which lines and branches the tests actually exercise. With
[`gcovr`](https://gcovr.com/) installed, `make coverage` does the whole flow,
rebuilding instrumented, running both test suites, and writing the report into
`build/`:

```sh
make coverage                       # -> build/coverage.txt and build/coverage.html
```

gcovr runs `gcov` in its own scratch directory, so no stray `.gcov` files land
in the repo root. The underlying steps, if you want to run them by hand:

```sh
make clean && make coverage=1
./tests/run.sh
gcovr --root . --exclude tests --print-summary   # per-file line/branch table
```

This is a *signal*, not a gate: it surfaces cold analyzer paths (a branch that
is detected but never exercised, versus one that is dead) rather than enforcing
a threshold. The `coverage` CI job runs the same flow and uploads an HTML
drill-down as a build artifact.
