#!/usr/bin/env bash
#
# Black-box correctness tests for sudoku-solver.
#
# These drive the compiled binary through its REPL (the same interface a user
# has) and check that it produces *correct* results, not merely that it doesn't
# crash (the latter is covered by the sanitizer smoke test in CI).
#
# Three things are checked:
#   [1] Full-solve correctness  -- known puzzles solve to their known grids.
#   [2] Soundness               -- the solver never writes a wrong value.
#   [3] Per-technique application-- advanced analyzers actually fire.
#
# Run from anywhere:  ./tests/run.sh         (uses ../sudoku-solver)
# Override binary:     SOLVER=/path/to/bin ./tests/run.sh
#
set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
SOLVER="${SOLVER:-$ROOT/sudoku-solver}"

if [ ! -x "$SOLVER" ]; then
    echo "FATAL: solver binary not found or not executable: $SOLVER" >&2
    echo "       build it first (e.g. 'make')." >&2
    exit 2
fi

pass=0
fail=0
ok()  { pass=$((pass + 1)); printf '  ok    %s\n' "$1"; }
bad() { fail=$((fail + 1)); printf '  FAIL  %s\n' "$1"; [ -n "${2:-}" ] && printf '        %s\n' "$2"; }

# Pull every 81-cell board snapshot out of solver output: strip spaces, keep
# lines that are exactly 81 chars of digits/dots (the 'p' command's format).
# Other output (ASCII board, status lines) contains brackets/letters and so is
# naturally excluded.
extract_grids() { tr -d ' ' | grep -E '^[1-9.]{81}$'; }

# ---------------------------------------------------------------------------
# Fixtures: puzzle -> known solution.
#
# Each solution was independently verified to be a legal completed grid (every
# row, column, and 3x3 box a permutation of 1-9) consistent with the puzzle's
# clues. EASY is the well-known Wikipedia sample puzzle; its solution here
# matches the published one.
# ---------------------------------------------------------------------------
P_easy="53..7....6..195....98....6.8...6...34..8.3..17...2...6.6....28....419..5....8..79"
S_easy="534678912672195348198342567859761423426853791713924856961537284287419635345286179"

P_med="300000000970010000600583000200000900500621003008000005000435002000090056000000001"
S_med="381976524975214638642583179264358917597621483138749265816435792423197856759862341"

P_clm="1.....569492.561.8.561.924...964.8.1.64.1....218.356.4.4.5...169.5.614.2621.....5"
S_clm="187423569492756138356189247539647821764218953218935674843592716975361482621874395"

# A hard puzzle (the "5*b" board from notes.txt) that fully solves but only by
# applying the advanced analyzers: simple coloring, Y-Wing and XY-chain all
# fire real eliminations. Doubles as a full-solve fixture and the SC/YW/XY
# per-technique source.
P_adv="19.342..52.581943.483...219..12.5..4..91.4.2.7426...51918....42.2.4..1933.4921.68"
S_adv="197342685265819437483756219631295874859174326742638951918563742526487193374921568"

# A harder puzzle the pure-logic solver does NOT fully crack, but along the way
# it is forced to *apply* X-Wing, naked-pair and locked-candidate eliminations.
P_hard="100400006046091200002000000300000040000208000060000005000000900008750120700003004"

echo "[1] Full-solve correctness"
for name in easy med clm adv; do
    pvar="P_$name"; svar="S_$name"
    out="$(printf 'n.%s\nr\np\n' "${!pvar}" | "$SOLVER" 2>&1)"
    got="$(printf '%s' "$out" | extract_grids | tail -1)"
    if ! printf '%s' "$out" | grep -q 'SOLVED!'; then
        bad "$name: solver did not reach SOLVED!"
    elif [ "$got" = "${!svar}" ]; then
        ok "$name: solved to the expected grid"
    else
        bad "$name: solved to the WRONG grid" "got: ${got:-<none>}"
    fi
done

echo "[2] Soundness: no cell is ever set to a value that contradicts the solution"
# Step one technique at a time, printing after each step, so we see every
# intermediate board. A correct solver only ever fills a cell with its true
# value; a single wrong placement here means an unsound deduction.
input="n.$P_med"$'\n'
for _ in $(seq 200); do input+=$'.\np\n'; done
snaps="$(printf '%s' "$input" | "$SOLVER" 2>&1 | extract_grids)"
offending=""
while IFS= read -r g; do
    [ -z "$g" ] && continue
    if ! awk -v g="$g" -v s="$S_med" '
        BEGIN { for (i = 1; i <= 81; i++) {
                    c = substr(g, i, 1)
                    if (c != "." && c != substr(s, i, 1)) exit 1
                } exit 0 }'; then
        offending="$g"; break
    fi
done <<< "$snaps"
if [ -n "$offending" ]; then
    bad "med: a snapshot placed a wrong value" "$offending"
elif [ -z "$snaps" ]; then
    bad "med: produced no board snapshots to check"
else
    ok "med: every intermediate placement is consistent with the solution"
fi

echo "[3] Per-technique: advanced analyzers actually apply eliminations"
# Applied steps look like '[XW] [3, 8] x9 [r]' (tag, space, coordinate).
# Detection-only summary lines look like '[XW](0) {}' (tag immediately followed
# by '('), and must NOT count -- they print every step regardless.
check_tech() { # $1 = solver verbose output, $2 = tag, $3 = human name
    if printf '%s' "$1" | grep -qE "^\[$2\] \[[0-9]"; then
        ok "$3 ($2): applied at least one elimination"
    else
        bad "$3 ($2): never applied (analyzer may be broken)"
    fi
}
vout_hard="$(printf 'v\nn.%s\nr\n' "$P_hard" | "$SOLVER" 2>&1)"
check_tech "$vout_hard" XW "X-Wing"
check_tech "$vout_hard" NP "Naked Pair"
check_tech "$vout_hard" LC "Locked Candidates"
vout_adv="$(printf 'v\nn.%s\nr\n' "$P_adv" | "$SOLVER" 2>&1)"
check_tech "$vout_adv" SC "Simple Coloring"
check_tech "$vout_adv" YW "Y-Wing"
check_tech "$vout_adv" XY "XY-Chain"

echo "[4] Load-time error messages for bad input"
# The smoke test in CI proves bad input does not crash; this pins the actual
# diagnostic text so the self-describing load errors cannot silently regress.
# $1 = description, $2 = REPL line, $3 = expected substring (fixed string).
expect_err() {
    out="$(printf '%s\n' "$2" | "$SOLVER" 2>&1)"
    if printf '%s' "$out" | grep -qF "$3"; then
        ok "$1"
    else
        bad "$1" "expected to see: $3"
    fi
}
expect_err "truncated board"     "n.123"                                  "expected 81 cells, got 3"
expect_err "oversized board"     "n.$(printf '1%.0s' $(seq 90))"          "expected 81 cells, got 90"
expect_err "invalid character"   "n.$(printf 'z%.0s' $(seq 81))"          "invalid character 'z' at position 1"
expect_err "contradictory board" "n.11......................................................................8........" "appears more than once in a row"
expect_err "bad leading char"    "n@nonsense"                             "board must start with"
expect_err "empty board"         "n"                                      "no board provided"
expect_err "form-1 bad entry"    "n;99"                                   'cannot parse entry "99"'
expect_err "cell set twice"      "n;111;112"                              "cell (1,1) set more than once"

echo
echo "----------------------------------------"
printf 'passed: %d   failed: %d\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
