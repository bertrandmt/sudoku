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

Mapping all ten techniques:

| Class | Pattern identity | Techniques | `test_`? |
|-------|------------------|------------|----------|
| **Given-tuple** | a small fixed-arity tuple the `find_` loop enumerates | naked single (1 cell), hidden single (cell+value), naked pair (2 cells), **hidden pair (2 cells + 2 values)**, Y-Wing (3 cells) | yes — natural |
| **Materialized-object** | a standalone object built by an independent discovery step | simple coloring (`ColorChain`), XY-chain (chain vector) | yes — natural |
| **Scan-fused** | membership emerges only from the validating scan; no tuple, no separable object | locked candidates, X-Wing, Swordfish | no — inline |

- **Given-tuple**: `find_` enumerates tuples (`for` over cells / value pairs);
  `test_` judges each. The would-act / confinement scan inside `test_` operates
  on an identity that already exists. `test_naked_pair(c1, c2, set)` is the
  archetype.

- **Materialized-object**: discovery and validation are genuinely distinct
  operations. `find_xychain` *builds* a chain by following links;
  `test_xychain` separately *scores* the eliminations that chain would make. The
  chain is a first-class value, so it is clean to pass to a predicate.

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
materialized-object) and 3 inline (the 3 scan-fused). The codebase very nearly
matches, with one exception:

- **Hidden pair is mis-classified.** It is given-tuple shaped (identity is
  `(c1, c2, v1, v2)`; the "no other cell in the unit carries v1 or v2" condition
  is just validation of a given tuple, exactly like naked pair's would-act
  check), yet it currently validates inline with hand-rolled
  `condition_met` / `ppair_cell` single-pass bookkeeping. It should be extracted
  to a `test_hidden_pair` mirroring `test_naked_pair`. Tracked as a separate
  issue.

- **X-Wing correctly has no `test_`.** It is scan-fused; PR #24 removed the dead
  predicate and kept `find_xwing` inline, tested through `find_` like its fish
  sibling Swordfish. This is the rule working as intended, not a coverage gap.

- **Locked candidates and Swordfish correctly inline.** Both are scan-fused; a
  `test_` for either would hit the out-param / re-derivation smell above.

## A note on templates

Of the techniques with a `test_`, only `test_ywing` is exercised *directly* by
the unit tests — and it is the only one that is **not** templated on the unit
type. The templated predicates (`test_naked_pair`, etc.) are tested only
*through* their `find_`. A templated `test_` called directly across translation
units risks GCC inlining it away with no emitted symbol, forcing explicit
instantiations (the link failure PR #24 hit and then shed by deleting the
predicate). So when extracting a templated predicate, decide up front: test it
through `find_` (cheap, coarser), or pay the explicit-instantiation tax for
direct per-branch predicate tests.
