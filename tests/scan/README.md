# Screenshot-scanning fixtures

Ground truth for the image scanner proposed in
[#68](https://github.com/bertrandmt/sudoku/issues/68): read a sudoku app
screenshot and print the canonical `n.` line, set cells recognized and candidate
cells emitted as `.`.

The images are committed **verbatim**, at their original dimensions and with their
original JPEG encoding untouched. They are not cropped to the grid and not
re-encoded, because the surrounding furniture and the compression artifacts are
part of what each fixture tests. Byte-identity with the capture was verified by
checksum at commit time.

Neither `make test` nor `make unit` reaches this directory. A scan check needs its
own entry point.

## How a label is established

An expected line here is *earned*, never hand-typed at volume. The gate:

1. It loads, i.e. `reject_duplicate_values` (`board.cpp`) does not throw.
2. No cell was flagged low-confidence by the scanner.
3. The solver's computed candidates agree with the notes the app displays, where
   the source's note rendering permits reading them.
4. A human confirms the printed grid once.

Each entry below records which of those actually hold for it today, since the
scanner does not exist yet and gate 2 cannot have run.

## 001-notes-lattice.jpeg

1206x1253, 211 KB. Unidentified app, blue and grey theme, header bar with mistake
count / difficulty / timer, 60 set cells and 21 note cells.

```
n.9256...71471....568637512942374195681598674326845..7197183....53..185..75...7.183
```

Solves to completion under `r`:

```
925634871 471298356 863751294 237419568 159867432 684523719 718346925 392185647 546972183
```

Notes render in a **3x3 lattice**, so a candidate's value is encoded by its slot
and needs no glyph recognition.

Label status: read by eye, loads, solves. Gate 3 is a hand spot-check only, not a
measurement, because the small glyphs in the lower-right nonets are exactly the
ones a human reads unreliably.

Hazards it covers:

* Cell-background fill highlighting every cell holding the selected value.
* Per-candidate highlight chips behind individual note glyphs.
* Row and column tinting.
* Application chrome above and below the grid.
* Givens and player entries in **different colors** (black and blue), so this is
  the source where a `--givens-only` mode is possible.

## 002-wood-tiles-reddit.jpeg

1206x1407, 409 KB. Unidentified app with a wooden-tile theme, dark red glyphs on
textured beige, captured from an r/sudoku post (third-party content, reproduced
here as test input). 66 set cells and 15 note cells.

```
n...3849.7...8.2694...9.57.8.897465321154293867236718594..5981236381672459962534718
```

Solves to completion under `r`:

```
623849175 578126943 419357682 897465321 154293867 236718594 745981236 381672459 962534718
```

Notes render as **packed variable-width digit strings** that wrap to a second line
(one cell shows `124` as `12` over `4`), so reading them requires recognizing the
small glyphs. Position encodes nothing.

Label status: read by eye, and gate 3 holds **mechanically** rather than by
squinting. The solver's candidate dump is `56 12 / 57 17 / 46 124 / 47 47` plus
`13 16 25 35 23`, which is character-for-character what the app displays in all
15 note cells, the wrapped `124` included.

Hazards it covers:

* Textured (wood grain) background, so no global light/dark ink threshold.
* Colored ink rather than black.
* Separators are bevels in nearly the tile's own tone, not dark rules, so grid
  detection cannot key on dark lines.
* Hostile surroundings: the board is roughly 75% of the frame, with post furniture
  around it, and the r/sudoku avatar in the corner is itself a small grid that a
  naive "largest grid-like region" search must reject on size.
* Givens and player entries in **one color**, so `--givens-only` is impossible
  here.

This board is also a useful analyzer datapoint independent of scanning. From the
position above, all ten other techniques go dry and the unstick is a single
XY-Chain: `r1c2{1,2}` to `r1c9{2,5}` to `r2c9{3,5}` to `r2c4{3,1}`, both ends
carrying 1, eliminating 1 from `r2c2{1,7}` which sees both ends. That placement
cascades to the full solve.
