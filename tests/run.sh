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

# Wall-clock guard for every solver invocation. A bug that loops forever inside
# a single step (a cycle in chain-building, say) would otherwise hang the whole
# run -- bounded *input* does not bound time spent in one step. With a timeout
# the hang turns into a truncated-output assertion failure in seconds instead.
# 'timeout' is GNU coreutils; macOS ships it as 'gtimeout' if coreutils is
# installed and otherwise lacks it, so degrade to a no-op there (the CI step's
# timeout-minutes is the backstop on that leg). Override the budget with
# SOLVER_TIMEOUT=<seconds>, or SOLVER_TIMEOUT=0 to disable.
SOLVER_TIMEOUT="${SOLVER_TIMEOUT:-20}"
if [ "$SOLVER_TIMEOUT" = 0 ]; then            RUN=""
elif command -v timeout  >/dev/null 2>&1; then RUN="timeout $SOLVER_TIMEOUT"
elif command -v gtimeout >/dev/null 2>&1; then RUN="gtimeout $SOLVER_TIMEOUT"
else                                           RUN=""; fi
# Run the solver under the guard. Used in a pipe (stdin and stdout flow through)
# everywhere a test drives the binary; the bare "$SOLVER" stays only in the
# existence check above.
run_solver() { $RUN "$SOLVER" "$@"; }

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

echo "[0] Fixture sanity: puzzles and solutions are well-formed before they gate the solver"
# A typo in a fixture would make a *correct* solver look broken, or mask a real
# bug behind a "wrong grid" that is actually the fixture's fault. So validate the
# fixtures themselves first, independently of the solver: every solution must be
# a legal completed grid (each row, column and 3x3 box a permutation of 1-9),
# every puzzle must be well-formed, and every solution must agree with its
# puzzle's given clues. A failure here is a fixture bug, reported as such.

# Validate a 9x9 grid string. mode "full" requires every cell filled with 1-9
# (a legal *completed* grid); mode "partial" allows '.'/'0' empties (a *puzzle*).
# Both modes reject any digit repeated within a row, column or box. Prints the
# first problem found and is silent on success.
grid_check() { # $1 = grid, $2 = mode (full|partial)
    awk -v g="$1" -v mode="$2" 'BEGIN {
        if (length(g) != 81) { print "length " length(g) " != 81"; exit 1 }
        for (i = 1; i <= 81; i++) {
            ch = substr(g, i, 1)
            if (ch == "." || ch == "0") {
                if (mode == "full") { print "empty cell at position " i; exit 1 }
            } else if (ch < "1" || ch > "9") {
                print "bad char \"" ch "\" at position " i; exit 1
            }
        }
        for (u = 0; u < 9; u++) {            # u indexes the u-th row, column and box
            split("", rs); split("", cs); split("", bs)
            for (k = 0; k < 9; k++) {        # k indexes the k-th cell within each
                rc = substr(g, u*9 + k + 1, 1)
                cc = substr(g, k*9 + u + 1, 1)
                bc = substr(g, (int(u/3)*3 + int(k/3))*9 + ((u%3)*3 + (k%3)) + 1, 1)
                if (rc != "." && rc != "0" && rs[rc]++) { print "row "    (u+1) " repeats " rc; exit 1 }
                if (cc != "." && cc != "0" && cs[cc]++) { print "column " (u+1) " repeats " cc; exit 1 }
                if (bc != "." && bc != "0" && bs[bc]++) { print "box "    (u+1) " repeats " bc; exit 1 }
            }
        }
        exit 0
    }'
}

# Every given clue in the puzzle must match the solution at that position.
consistent() { # $1 = puzzle, $2 = solution
    awk -v p="$1" -v s="$2" 'BEGIN {
        for (i = 1; i <= 81; i++) {
            pc = substr(p, i, 1)
            if (pc == "." || pc == "0") continue
            sc = substr(s, i, 1)
            if (pc != sc) { print "position " i ": clue " pc " but solution has " sc; exit 1 }
        }
        exit 0
    }'
}

fixture_ok() { # $1 = name, $2 = puzzle, $3 = solution
    local why
    why="$(grid_check "$2" partial)"; if [ -n "$why" ]; then bad "$1: malformed puzzle fixture" "$why"; return; fi
    why="$(grid_check "$3" full)";    if [ -n "$why" ]; then bad "$1: solution is not a legal completed grid" "$why"; return; fi
    why="$(consistent "$2" "$3")";    if [ -n "$why" ]; then bad "$1: solution contradicts a puzzle clue" "$why"; return; fi
    ok "$1: puzzle and solution are well-formed and consistent"
}
for name in easy med clm adv; do
    pvar="P_$name"; svar="S_$name"
    fixture_ok "$name" "${!pvar}" "${!svar}"
done
# P_hard has no solution fixture; just confirm the puzzle itself is well-formed.
why_hard="$(grid_check "$P_hard" partial)"
if [ -n "$why_hard" ]; then bad "hard: malformed puzzle fixture" "$why_hard"
else                        ok  "hard: puzzle is well-formed"; fi

echo "[1] Full-solve correctness"
for name in easy med clm adv; do
    pvar="P_$name"; svar="S_$name"
    out="$(printf 'n.%s\nr\np\n' "${!pvar}" | run_solver 2>&1)"
    got="$(printf '%s' "$out" | extract_grids | tail -1)"
    if ! printf '%s' "$out" | grep -q 'SOLVED!'; then
        bad "$name: solver did not reach SOLVED!"
    elif [ "$got" = "${!svar}" ]; then
        ok "$name: solved to the expected grid"
    else
        bad "$name: solved to the WRONG grid" "got: ${got:-<none>}"
    fi
done

echo "[2] Soundness: no step ever places a wrong value or removes a true candidate"
# The core correctness property: every cell must always still list its true
# solution digit. Step one technique at a time, dumping the machine-readable
# candidate grid ('c') after each step, and require every cell to keep listing
# its solution digit throughout. The single check "solution digit is present in
# the cell's field" catches both failure modes at once: a wrong placement (the
# placed digit replaces the whole field, dropping the true digit) and an
# over-elimination (a true candidate struck from a still-unsolved cell). The
# latter is the subtle one -- it usually does not produce a wrong final grid, it
# just stalls the puzzle into '???' -- and it is caught here at the exact step
# that causes it. Runs on every fixture with a known solution, so the advanced
# analyzers (SC/YW/XY via adv; LC/NP via clm) are all exercised.
# soundness_violation is the shared engine behind tier [2] and the corpus tier
# [6]. It steps one technique at a time, dumping the machine-readable candidate
# grid ('c') after each step, and checks that every cell still lists its
# solution digit throughout. Echoes a human-readable description of the first
# violation found, the sentinel "NO_GRIDS" if the solver emitted no candidate
# grids at all, or nothing if every step was sound.
soundness_violation() { # $1 = puzzle, $2 = solution
    local input="n.$1"$'\n'
    local _
    for _ in $(seq 200); do input+=$'.\nc\n'; done
    local out; out="$(printf '%s' "$input" | run_solver 2>&1)"
    if [ -z "$(printf '%s' "$out" | grep '^~')" ]; then echo "NO_GRIDS"; return; fi
    # Each '~' line is one logical row of the candidate grid; the solver emits
    # rows 0-8 per snapshot, so the row index is just (line number - 1) % 9.
    # Field $1 is the '~' sentinel; $2..$10 are the nine cells of that row.
    printf '%s' "$out" | grep '^~' | awk -v s="$2" '
        { r = (NR - 1) % 9
          for (c = 0; c < 9; c++) {
              field = $(c + 2)
              d = substr(s, r * 9 + c + 1, 1)
              if (index(field, d) == 0) {
                  printf "cell (%d,%d): solution %s gone, field {%s}", r+1, c+1, d, field
                  exit
              }
          }
        }'
}

elim_check() { # $1 = name, $2 = puzzle, $3 = solution
    local v; v="$(soundness_violation "$2" "$3")"
    if   [ "$v" = NO_GRIDS ]; then bad "$1: produced no candidate grids to check"
    elif [ -n "$v" ];         then bad "$1: an unsound step dropped a true candidate" "$v"
    else                           ok  "$1: every candidate grid still lists the solution's digits"
    fi
}
for name in easy med clm adv; do
    pvar="P_$name"; svar="S_$name"
    elim_check "$name" "${!pvar}" "${!svar}"
done

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
vout_hard="$(printf 'v\nn.%s\nr\n' "$P_hard" | run_solver 2>&1)"
check_tech "$vout_hard" XW "X-Wing"
check_tech "$vout_hard" NP "Naked Pair"
check_tech "$vout_hard" LC "Locked Candidates"
vout_adv="$(printf 'v\nn.%s\nr\n' "$P_adv" | run_solver 2>&1)"
check_tech "$vout_adv" SC "Simple Coloring"
check_tech "$vout_adv" YW "Y-Wing"
check_tech "$vout_adv" XY "XY-Chain"

echo "[4] Load-time error messages for bad input"
# The smoke test in CI proves bad input does not crash; this pins the actual
# diagnostic text so the self-describing load errors cannot silently regress.
# $1 = description, $2 = REPL line, $3 = expected substring (fixed string).
expect_err() {
    out="$(printf '%s\n' "$2" | run_solver 2>&1)"
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

echo "[5] State management: determinism, undo and reset"
# Board-grid lines start with '[' and contain '|'. Action-annotation lines such
# as '[NS] [2, 4] =2' also start with '[' but contain no '|', so they are
# excluded: we compare board *state*, not the step that produced it.
board_after() { # $1 = full output, $2 = marker -> board grid printed just before #<marker>
    printf '%s' "$1" | awk -v m="#$2" '
        /^#/      { if ($0 == m) { printf "%s", buf; exit } buf = ""; next }
        /^\[.*\|/ { buf = buf $0 "\n" }'
}

# Determinism: the same puzzle solved twice yields byte-identical output. (The
# analyzers use unordered_set; this guards against iteration-order leaking out.)
d1="$(printf 'n.%s\nr\n' "$P_med" | run_solver 2>&1)"
d2="$(printf 'n.%s\nr\n' "$P_med" | run_solver 2>&1)"
if [ "$d1" = "$d2" ]; then ok "determinism: identical output across two runs"
else bad "determinism: output differed between runs"; fi

# Undo: step forward twice, then back twice; each step back must reproduce the
# earlier board exactly. A marker after every command isolates one board each.
u="$(printf 'n.%s\n#S0\n.\n#S1\n.\n#S2\n<\n#B1\n<\n#B0\n' "$P_med" | run_solver 2>&1)"
us0="$(board_after "$u" S0)"; us1="$(board_after "$u" S1)"
ub1="$(board_after "$u" B1)"; ub0="$(board_after "$u" B0)"
if [ "$us0" = "$us1" ]; then bad "undo: forward steps did not change the board (test is vacuous)"
elif [ "$ub1" != "$us1" ]; then bad "undo: one step back does not match the prior state"
elif [ "$ub0" != "$us0" ]; then bad "undo: two steps back does not match the initial state"
else ok "undo: stepping back reproduces prior board states"; fi

# Reset: from a stepped position, '!' restores the initial board. The marker
# right before '!' isolates the reset board from the intervening step boards.
r="$(printf 'n.%s\n#R0\n.\n.\n#PRE\n!\n#RST\n' "$P_med" | run_solver 2>&1)"
if [ "$(board_after "$r" RST)" = "$(board_after "$r" R0)" ]; then
    ok "reset: '!' restores the initial board"
else
    bad "reset: '!' did not restore the initial board"
fi

echo "[6] Corpus: every puzzle logged in notes.txt solves correctly and soundly"
# notes.txt is a hand-kept log of real puzzles, one per line as
# "<label> - <board> - [techniques]". It is reference data, not a faithful
# trace of *this* solver: spot checks show several puzzles annotated [LC] that
# the solver reaches by another path, and one transcription typo (1/29/25
# repeats a value in a column), so the hand annotations are deliberately NOT
# asserted here. What we *can* assert, independent of any annotation, is the
# property that actually matters: any puzzle the solver finishes must finish at
# a legal grid consistent with its clues, and must never drop a true candidate
# on the way. That turns the ~25 real puzzles in notes.txt into full-solve and
# per-step soundness fixtures for free, with no hand-written solution strings --
# the solver's own final grid is validated structurally (a wrong placement would
# make it fail grid_check) and only then reused as the soundness oracle, so the
# oracle cannot certify a broken solve. Puzzles flagged "unsolved" must NOT be
# solved (a guard against an over-eager analyzer); a malformed board is reported
# and skipped rather than blamed on the solver.
NOTES="$ROOT/notes.txt"
if [ ! -f "$NOTES" ]; then
    bad "corpus: notes.txt not found at $NOTES"
else
    # One tab-separated record per puzzle line: index, label, board, solvable.
    # A line is a puzzle iff its second " - "-delimited field, with spaces and a
    # trailing " -" stripped, is exactly 81 chars of [1-9.]; section headers,
    # URLs and blank lines have no such field and fall through.
    corpus_records="$(awk -F ' - ' '
        { board = $2
          gsub(/[ -]+$/, "", board); gsub(/ /, "", board)
          if (board !~ /^[1-9.]{81}$/) next
          solvable = (index($0, "unsolved") == 0) ? "Y" : "N"
          label = $1; gsub(/[ \t]+$/, "", label)
          printf "%02d\t%s\t%s\t%s\n", ++n, label, board, solvable
        }' "$NOTES")"

    # A here-string (not a pipe) keeps the loop in this shell so ok/bad update
    # the pass/fail counters; a piped while-read would tally them in a subshell.
    while IFS=$'\t' read -r idx label board solv; do
        [ -n "$board" ] || continue
        name="$idx $label"
        why="$(grid_check "$board" partial)"
        if [ -n "$why" ]; then
            printf '  skip  %s: malformed board in notes.txt (%s)\n' "$name" "$why"
            continue
        fi
        out="$(printf 'n.%s\nr\np\n' "$board" | run_solver 2>&1)"
        if [ "$solv" = N ]; then
            if printf '%s' "$out" | grep -q 'SOLVED!'; then
                bad "$name: flagged unsolved in notes but the solver reports SOLVED"
            else
                ok "$name: correctly left unsolved (no false solve)"
            fi
            continue
        fi
        if ! printf '%s' "$out" | grep -q 'SOLVED!'; then
            bad "$name: solver did not reach SOLVED!"; continue
        fi
        grid="$(printf '%s' "$out" | extract_grids | tail -1)"
        why="$(grid_check "$grid" full)"
        if [ -n "$why" ]; then bad "$name: solved to an illegal grid" "$why"; continue; fi
        why="$(consistent "$board" "$grid")"
        if [ -n "$why" ]; then bad "$name: solution contradicts a clue" "$why"; continue; fi
        v="$(soundness_violation "$board" "$grid")"
        if   [ "$v" = NO_GRIDS ]; then bad "$name: produced no candidate grids to check"
        elif [ -n "$v" ];         then bad "$name: an unsound step dropped a true candidate" "$v"
        else ok "$name: solves to a legal grid consistent with its clues; every step sound"
        fi
    done <<< "$corpus_records"
fi

echo "[7] Per-technique precision: each advanced analyzer's first application eliminates exactly the expected candidates"
# Tier [3] only proves a technique fires *at all*; it cannot catch under-firing
# (pattern detected but half its eliminations missed) or a quietly changed
# result. This pins the exact set of candidates removed by the *first*
# application of each advanced technique on a chosen fixture. One solver step
# applies exactly one technique, so a step is one coherent application.
#
# Lines are compared as a sorted *set*: act_on_* applies every found instance
# (NP/LC/HP/XW/YW) or a single colour-swap-invariant chain (SC), so the set is
# independent of hash-table iteration order -- which differs between libstdc++
# and libc++; only the print order does, and LC_ALL=C sort absorbs that. (XY
# picks the best chain from an ordered set; unique for this fixture.) These
# goldens are regression locks, not an independent oracle: their *soundness* is
# guaranteed by tiers [2]/[6], which solve the same fixtures, so a wrong
# elimination could never be locked in here -- it would fail there first. If a
# legitimate path change ever alters a block, the diff below shows exactly what
# moved so the golden can be re-reviewed and updated.
P_color="289...375364.9.812517283964893.2.6.1145836729726....83451378296.72.1..38.38..21.7"
P_yw1="..28.4..1..4.6.2.887.32.4.5923618..44.5...6.37..543.29258.37.46649.8.3.71374.6..2"
P_xy2=".......9476.91..5..9...2.81.7..5..1....7.9....8..31.6724.1...7..1..9..459.....1.."
first_app() { # stdin = verbose solve output; $1 = tag -> sorted elimination block
    awk -v tag="$1" '
        /^Step #/                   { if (printed) exit; next }
        $0 ~ ("^\\[" tag "\\] \\[") { print; printed = 1 }
    ' | LC_ALL=C sort
}
prec_check() { # $1 = name, $2 = tag, $3 = board, $4 = expected sorted block
    local got
    got="$(printf 'v\nn.%s\nr\n' "$3" | run_solver 2>&1 | first_app "$2")"
    if   [ -z "$got" ];     then bad "$1 ($2): technique never applied"
    elif [ "$got" = "$4" ]; then ok  "$1 ($2): first application eliminates exactly the expected candidates"
    else bad "$1 ($2): first-application eliminations changed" "$(printf 'expected:\n%s\n--- got:\n%s' "$4" "$got")"
    fi
}
prec_check "naked pair"        NP "$P_color" "[NP] [6, 4] x4 [r]
[NP] [6, 4] x5 [r]
[NP] [6, 6] x4 [r]
[NP] [6, 6] x5 [r]"
prec_check "locked candidates" LC "$P_adv" "[LC] [3, 5] x7 [c]
[LC] [8, 5] x7 [c]"
prec_check "hidden pair"       HP "$P_clm" "[HP] [4, 8] x3 {[4, 8],[5, 8]}#{2,5}
[HP] [4, 8] x7 {[4, 8],[5, 8]}#{2,5}
[HP] [5, 8] x3 {[4, 8],[5, 8]}#{2,5}
[HP] [5, 8] x7 {[4, 8],[5, 8]}#{2,5}
[HP] [5, 8] x9 {[4, 8],[5, 8]}#{2,5}"
prec_check "x-wing"            XW "$P_clm" "[XW] [1, 4] x7 [c]
[XW] [5, 4] x7 [c]
[XW] [8, 4] x7 [c]
[XW] [8, 8] x7 [c]
[XW] [9, 4] x7 [c]
[XW] [9, 8] x7 [c]"
prec_check "simple coloring"   SC "$P_color" "[SC] [9, 5] x4 [👀🟩🟥]"
prec_check "y-wing"            YW "$P_yw1" "[YW] [1, 5] x9
[YW] [2, 8] x9"
prec_check "xy-chain"          XY "$P_xy2" "[XY] [5, 5] x2 ({[5, 8]:..:[6, 4]}#2)
[XY] [6, 7] x2 ({[5, 8]:..:[6, 4]}#2)"

echo
echo "----------------------------------------"
printf 'passed: %d   failed: %d\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
