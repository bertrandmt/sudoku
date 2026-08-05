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

Mapping all eleven techniques:

| Class | Pattern identity | Techniques | `test_`? |
|-------|------------------|------------|----------|
| **Given-tuple** | a small fixed-arity tuple the `find_` loop enumerates | naked single (1 cell), hidden single (cell+value), naked pair (2 cells), **hidden pair (2 cells + 2 values)**, Y-Wing (3 cells) | yes — natural |
| **Materialized-object** | a standalone object built by an independent discovery step | simple coloring (`ColorChain`), XY-chain (chain vector) | yes — natural |
| **Scan-fused** | membership emerges only from the validating scan; no tuple, no separable object | locked candidates, X-Wing, Swordfish, finned X-Wing | no — inline |

- **Given-tuple**: `find_` enumerates tuples (`for` over cells / value pairs);
  `test_` judges each. The would-act / confinement scan inside `test_` operates
  on an identity that already exists. `test_naked_pair(c1, c2, set)` is the
  archetype.

- **Materialized-object**: discovery and validation are genuinely distinct
  operations. `find_xychain` *builds* a chain by following links;
  `test_xychain` separately *scores* the eliminations that chain would make. The
  chain is a first-class value, so it is clean to pass to a predicate.
  (`test_xychain` returns the elimination count rather than a `bool`, because
  XY-chain also uses that count to rank competing chains. It is still a pure
  predicate over a materialized object; "actionable" is just `> 0`.)

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

The partition the rule predicts is 7 with `test_` (5 given-tuple + 2
materialized-object) and 4 inline (the 4 scan-fused). The codebase matches:

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
  could re-derive. It takes the same scan-fused seam shape as the other two fish
  (`FinnedXWingTechnique::find_finned_xwing` public `static`, `FinnedXWingFinding`
  in `analyzer-finnedxwing.h`) because its whitebox cases read the recorded fins.

- **XY-chain has a `test_`, and keeps it private.** It is materialized-object
  shaped like simple coloring, so `test_xychain` is the right factoring and
  survives the port (issue #7) as a file-local function in
  `analyzer-xychain.cpp`. It is *not* promoted, because no whitebox case calls
  it: promoting it would advertise a tested contract nothing tests. What the
  cases do drive is the per-anchor search `XYChainTechnique::find_xychain` and
  the best-chain selection `XYChainTechnique::record_if_best`, so those are the
  public statics, and `XYChainFinding` is in the header because a case reads the
  recorded chain's fields. The seam shape follows what the tests reach for, not
  the technique's class — the same "exposed exactly when the test must read it"
  rule that governs the findings.

  `record_if_best` is the one seam here with no analogue in the other nine.
  XY-chain is the solver's only find-many/act-one technique: it ranks every chain
  it discovers and applies just the most desirable one, so the ranking rule is a
  contract in its own right, and one a crafted board cannot isolate (it takes
  several competing chains, and which chains a board yields is not the case's to
  dictate). The case offers them to the selector directly instead.

  That shape is described here, not endorsed. The design intent for the cascade
  is that the cheapest firing technique is applied *as greedily as possible*, and
  XY-chain is the one tier that isn't — **issue #36** is open to make it act on
  every distinct elimination effect. That would delete `record_if_best` and the
  finding's `operator==` (endpoint equivalence being only an inexact proxy for
  "same elimination set"), and would have `test_xychain` collect the eliminated
  coords rather than count them. It would *not* necessarily touch `operator<`:
  #36's open question is whether to keep the coverage-maximal chain, which is
  what this comparator already ranks first, or the shortest chain per
  elimination, and it is deliberately left undecided there. The port preserved
  existing behavior byte-for-byte, so it promoted that behavior into a named
  seam; that is a consequence of the port's terms, not a ruling on #36.

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
  recorded here only so an old commit reads clearly: with all ten techniques
  ported there is no longer a predicate that could need it, and a new technique
  is a standalone `Technique` from its first line.

- **The decision.** When extracting a templated predicate, choose up front: test
  it through `find_` (cheap, coarser, no link tax), or pay the
  explicit-instantiation tax (plus a friend hook, if the technique is still
  private to `Analyzer`) for direct per-branch predicate tests. Both worked
  examples, driven from `tests/unit/test_analyzer.cpp`, took the second route and
  are now the promoted public-static-member variant: `test_naked_pair`
  (`analyzer-nakedpairs.h` / `.cpp`) and `test_hidden_pair`
  (`analyzer-hiddenpairs.h` / `.cpp`). Both used the friend-hook variant while
  private to `Analyzer` and shed it on porting (issue #7).
