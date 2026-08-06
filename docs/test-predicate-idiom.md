# When a technique gets a `test_` predicate (and when it doesn't)

Every solving technique in the analyzer follows a `find_` / `act_on_` split.
Some techniques *also* factor their validation into a separate `test_`
predicate (`test_naked_pair`, `test_ywing`, `test_hidden_single`, ...); others
validate inline inside `find_` (locked candidates, X-Wing, Swordfish). This is a
recurring source of "why doesn't X-Wing have a `test_` like Y-Wing does?"
confusion. The split is **principled, not incidental**. This note records the
rule so it isn't re-litigated.

## The rule

A `test_` predicate is the right factoring exactly when **the pattern has an
identity that exists prior to, and independently of, its validation** — so the
`find_` loop can name the candidate pattern, hand it to a pure `const` predicate,
and let the predicate answer "is this a real, actionable instance?"

It is the *wrong* factoring when **the pattern's membership only emerges from the
validating scan itself** — there is nothing to hand in, because finding the cells
and validating them are the same operation.

## The three classes

Mapping every technique in the cascade:

| Class | Pattern identity | Techniques | `test_`? |
|-------|------------------|------------|----------|
| **Given-tuple** | a small fixed-arity tuple the `find_` loop enumerates | naked single (1 cell), hidden single (cell+value), naked pair (2 cells), **hidden pair (2 cells + 2 values)**, Y-Wing (3 cells) | yes — natural |
| **Materialized-object** | a standalone object built by an independent discovery step | simple coloring (`ColorChain`), XY-chain (chain vector) | yes — natural |
| **Scan-fused** | membership emerges only from the validating scan; no tuple, no separable object | locked candidates, and all four fish: X-Wing, Swordfish, finned X-Wing, finned Swordfish | no — inline |

- **Given-tuple**: `find_` enumerates tuples (`for` over cells / value pairs);
  `test_` judges each. The would-act / confinement scan inside `test_` operates
  on an identity that already exists. `test_naked_pair(c1, c2, set)` is the
  archetype.

- **Materialized-object**: discovery and validation are genuinely distinct
  operations. `find_xychain` *builds* a chain by following links;
  `test_xychain` separately *scores* the eliminations that chain would make. The
  chain is a first-class value, so it is clean to pass to a predicate.
  (`test_xychain` returns the eliminated coords rather than a `bool`, because
  XY-chain compares competing chains by the effect they would have and its
  `apply` replays that effect. It is still a pure predicate over a materialized
  object; "actionable" is just "non-empty".)

- **Scan-fused**: the act of checking the pattern condition *is* the act of
  discovering which cells belong to it. A locked candidate is "every candidate
  for V in this line happens to also lie in that intersection" — you cannot
  enumerate candidate patterns and test them; you discover the pattern *by*
  testing confinement. There is no fixed-arity tuple and no object produced by a
  separate build step.

## The diagnostic smell

If you try to extract a `test_` from a scan-fused technique, the code tells you
it doesn't fit:

- **An out-parameter that leaks discovered membership.** A
  `test_locked_candidate(...)` cannot return a bare `bool`: the caller also needs
  *which cells* it found, so the predicate must surface them through an out-param.
  That out-param is the signal that discovery and validation were never separate.

- **Re-deriving cells inside the predicate.** The original (dead, now removed)
  `test_xwing` took the candidate *sets* and re-derived the corner cells
  internally, redundantly with `find_xwing`, which needed those same corners to
  record the pattern. Wiring it in caused a measurable allocation regression for
  exactly that reason (see PR #24).

When you reach for either workaround, you are forcing a `test_` onto an algorithm
whose shape doesn't want one. Validate inline and drive the whitebox tests
through `find_` instead (this is what Swordfish and X-Wing do: craft a board,
call `find_`, inspect the recorded pattern).

## Current state vs. the rule

The partition the rule predicts is seven with `test_` (five given-tuple + two
materialized-object) and the rest inline: every scan-fused technique, which is
locked candidates plus all four fish. The codebase matches. The totals are spelled
out once, here, rather than restated per class — a bare count repeated in several
places is exactly what went stale when the cascade last grew:

- **Hidden pair is correctly factored.** It is given-tuple shaped (identity is
  `(c1, c2, v1, v2)`; the "no other cell in the unit carries v1 or v2" condition
  is just validation of a given tuple, exactly like naked pair's would-act
  check), and it is extracted to a `test_hidden_pair` mirroring `test_naked_pair`
  (the old hand-rolled `condition_met` / `ppair_cell` single-pass bookkeeping is
  gone). It has now ported behind the registry (issue #7), which -- exactly as it
  did for naked pair -- let its templated predicate promote to a public `static`
  member of `HiddenPairTechnique` and drop the friend hook; the whitebox suite
  calls `HiddenPairTechnique::test_hidden_pair` directly, backed by an explicit
  `Row` instantiation (see "A note on templates" below).

- **X-Wing correctly has no `test_`.** It is scan-fused; PR #24 removed the dead
  predicate and kept `find_xwing` inline, tested through `find_` like its fish
  sibling Swordfish. This is the rule working as intended, not a coverage gap. It
  has now ported behind the registry (issue #7). Because it is scan-fused there
  is no predicate to promote, so its whitebox seam differs from the given-tuple
  techniques: instead of a promoted `test_` predicate, the per-anchor entry
  `XWingTechnique::find_xwing` is promoted to a public `static` the test drives
  directly (static because the technique is stateless and the entry touches no
  instance data -- the same reason the given-tuple predicates are static), and the
  concrete `XWingFinding` is declared in `analyzer-xwing.h` rather than file-local
  so the test can inspect the recorded pattern's orientation and value. This is
  the *scan-fused* seam shape (finding-in-header + public `find_`), distinct from
  the *given-tuple* seam (promoted `test_` predicate, finding stays file-local);
  the finding is exposed exactly when the test must read its fields, not merely
  because the technique is hooked. Either way the friend hook is gone.

- **Locked candidates and Swordfish correctly inline.** Both are scan-fused; a
  `test_` for either would hit the out-param / re-derivation smell above.
  Swordfish has now ported behind the registry (issue #7) and takes the same
  *scan-fused* seam shape as its fish sibling X-Wing, for the same reason: no
  predicate to promote, so `SwordfishTechnique::find_swordfish` is the public
  `static` the test drives and `SwordfishFinding` is declared in
  `analyzer-swordfish.h` so the test can read the recorded pattern's orientation
  and value. Locked candidates ported too, but its coverage is entirely
  black-box (`tests/run.sh`); it has no whitebox cases, so it needed no seam at
  all.

- **Finned X-Wing joins them, and is the first technique added *after* the
  registry rather than ported onto it.** Scan-fused for a sharper reason than its
  siblings: it does not merely discover which cells belong to the pattern while
  validating it, it discovers *which two cross lines the pattern is even about*.
  The cover is chosen from the union rather than derived from it, so there is no
  candidate identity to hand a predicate -- not even the ambiguous one X-Wing
  could re-derive. It takes the same scan-fused seam shape as its fellow fish
  (`FinnedXWingTechnique::find_finned_xwing` public `static`, `FinnedXWingFinding`
  in `analyzer-finnedxwing.h`) because its whitebox cases read the recorded fins.

- **Finned Swordfish is that same technique one size up, and takes that same seam.**
  Scan-fused for the same sharper reason: the cover is *chosen*, as a three-line
  subset of the union rather than derived from it, so there is no candidate identity
  to hand a predicate. Seam:
  `FinnedSwordfishTechnique::find_finned_swordfish` public `static`, and
  `FinnedSwordfishFinding` in `analyzer-finnedswordfish.h` because its whitebox cases
  read the recorded fins. Its cases carry one wrinkle the other fish's do not. At
  three base lines the elimination scan must exclude *three* base lines, and only two
  of them can have a cell inside the fin's nonet at once — the nonet's band is three
  lines wide and one of those lines must be a non-base line for anything to be
  eliminable. So the duty is split: the row-based accept case witnesses the second and
  third base lines, the column-based one the first and second, and a third case covers
  the first base line inside the has-eliminations scan, where a genuine elimination
  would otherwise short-circuit the check before the base-line test is reached.

  Since #58 that exclusion is a fold over the base lines in one shared worker rather
  than a hand-written chain of terms per fish, so "which case witnesses which term" is
  no longer the right question — a fold cannot be written with a term missing. The
  split-duty geometry above still describes what the *positions* reach, and the cases
  are worth keeping for it: narrowing the fold to skip its first base line is caught
  by the column-based accept case and by the first-base-line case, and narrowing it to
  skip the last is caught by the row-based accept case, each still failing on the cell
  it was built to protect. Both mutations are also caught by the plain fish's cases
  now, which they were not when the two halves of the family had their own copies.

- **XY-chain has a `test_`, and keeps it private.** It is materialized-object
  shaped like simple coloring, so `test_xychain` is the right factoring and
  survives the port (issue #7) as a file-local function in
  `analyzer-xychain.cpp`. It is *not* promoted, because no whitebox case calls
  it: promoting it would advertise a tested contract nothing tests. What the
  cases do drive is the per-anchor search `XYChainTechnique::find_xychain` and
  the retention rule `XYChainTechnique::record_if_maximal`, so those are the
  public statics, and `XYChainFinding` is in the header because a case reads the
  recorded chain's fields. The seam shape follows what the tests reach for, not
  the technique's class — the same "exposed exactly when the test must read it"
  rule that governs the findings.

  `record_if_maximal` is the one seam here with no analogue in any other
  technique. Every other technique either records each occurrence unconditionally
  or stops at the first, so there is no rule to test; XY-chain searches
  exhaustively and then has to decide which of many overlapping findings are worth
  keeping. That rule is a contract in its own right, and one a crafted board
  cannot isolate (it takes several competing chains with chosen effects, and which
  chains a board yields is not the case's to dictate). The case offers them to it
  directly instead.

  The seam used to be `record_if_best`, a trim to the single most desirable chain,
  and it was described here as shape rather than endorsed: the design intent for
  the cascade is that the cheapest firing technique is applied *as greedily as
  possible*, and XY-chain was the one tier that wasn't. Issue #36 closed that gap.
  `record_if_best` and the finding's `operator==` are gone (endpoint equivalence
  being only an inexact proxy for "same elimination set"), `test_xychain` collects
  the eliminated coords rather than counting them, and the desirability
  `operator<` is gone with the ranking it existed to express — #36's open question
  (coverage-maximal versus shortest-per-elimination) was settled toward
  inclusion-maximal effects, each represented by its shortest chain, which needs
  subsumption and a tie-break rather than a total ranking.

## A note on templates

Most `test_` predicates are templated on the unit type (`test_naked_pair`, etc.);
`test_ywing` is the lone non-templated one. Testing a templated predicate
*directly* (not just through its `find_`) has a known sharp edge, now settled by
PR #27 for `test_naked_pair`:

- **The failure mode.** A templated `test_` whose only in-TU caller is its
  `find_` gets that one use inlined by g++ at `-O3`, so no out-of-line symbol is
  emitted. A unit test referencing the predicate across the TU boundary then
  fails to link — but *only on gcc*: clang keeps a weak definition, so the build
  passes locally and on the macos/linux-clang legs and breaks solely on
  linux-gcc. (PR #24 hit the same link failure from the other direction and shed
  it by deleting a dead predicate.)

- **The fix, when you want direct per-branch tests.** Whatever route the test
  uses to reach the predicate, add an explicit instantiation next to the
  definition so a standalone symbol is emitted regardless of inlining — this is
  the part that fixes the gcc-only link failure, and it is needed either way.
  Then promote the predicate to a public `static` member of the technique — a
  documented tested contract — and have the test call it directly, no friendship
  required. Naked pair took this route (issue #7): the test calls
  `NakedPairTechnique::test_naked_pair<Row>(...)` and the friend hook is deleted.
  Hidden pair took the same route (`HiddenPairTechnique::test_hidden_pair<Row>(...)`).
  Because the member is `static` there is no trailing `const`, and the explicit
  instantiation is likewise unqualified by `Analyzer`:
  `template bool NakedPairTechnique::test_naked_pair<Row>(const Cell &, const Cell &, const Row &);`.
  Confirm on CI, not locally: the Apple-clang `g++` shim cannot reproduce the
  gcc-only link failure.

  Before the registry there was a second route, for a predicate still private to
  `Analyzer`: a friend hook in `AnalyzerTest` instantiating it on a concrete unit
  (a `test_foo_row` wrapper calling `a.test_foo(..., a.mBoard.row(c1))`), with
  `template bool Analyzer::test_foo<Row>(...) const;` beside the definition. Both
  naked pair and hidden pair took it while private and shed it on porting. It is
  recorded here only so an old commit reads clearly: with the port complete there
  is no longer a predicate that could need it, and a new technique is a standalone
  `Technique` from its first line.

- **The decision.** When extracting a templated predicate, choose up front: test
  it through `find_` (cheap, coarser, no link tax), or pay the
  explicit-instantiation tax (plus a friend hook, if the technique is still
  private to `Analyzer`) for direct per-branch predicate tests. Both worked
  examples, driven from `tests/unit/test_analyzer.cpp`, took the second route and
  are now the promoted public-static-member variant: `test_naked_pair`
  (`analyzer-nakedpairs.h` / `.cpp`) and `test_hidden_pair`
  (`analyzer-hiddenpairs.h` / `.cpp`). Both used the friend-hook variant while
  private to `Analyzer` and shed it on porting (issue #7).
