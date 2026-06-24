# Security Policy

`sudoku-solver` is a command-line tool that reads a Sudoku board and solving
commands from standard input and prints results to standard output. It is a
local, single-user utility: it opens no network sockets, runs no privileged
operations, and writes only to the files you explicitly point it at (e.g. its
REPL history file). The realistic security surface is therefore limited to how
it handles untrusted input.

## Supported Versions

This is a hobby project maintained by a single author. Security fixes are
applied to the latest commit on the `main` branch only. There are no long-term
support branches or back-ports to tagged releases.

| Version        | Supported          |
| -------------- | ------------------ |
| `main` (latest)| :white_check_mark: |
| older commits  | :x:                |

## Threat Model

In scope (please report):

- **Memory-safety bugs** reachable from input: buffer overruns, out-of-bounds
  reads/writes, use-after-free, or undefined behavior triggered by a crafted
  board string or REPL command (the `n.` board loader and the command parser
  are the primary parsing entry points).
- **Crashes / denial of service** on malformed, oversized, or adversarial
  input that a normal user could plausibly feed the tool.
- **Unsafe handling of the history file** or any other path the tool reads or
  writes.

Out of scope:

- Producing an incorrect or incomplete *solve* for a valid board. That is a
  correctness bug, not a security issue: please open a normal GitHub issue.
- Resource use that only manifests on deliberately enormous input far beyond a
  9x9 board, where the fix would be a simple input-size cap.
- Issues requiring the attacker to already have write access to the source
  tree, the build toolchain, or the user's account.

## Reporting a Vulnerability

Please **do not** open a public GitHub issue for a suspected vulnerability.

Instead, use one of the following private channels:

1. **GitHub private vulnerability reporting** (preferred): on the repository's
   **Security** tab, choose **Report a vulnerability**. This opens a private
   advisory visible only to the maintainer.
2. **Email**: write to <sudoku-solver-security-issue@bmt-online.org> with a
   subject line beginning `[SECURITY]`.

When reporting, please include:

- The exact input or sequence of REPL commands that triggers the issue.
- The commit hash (`git rev-parse HEAD`), compiler, and platform you built on.
- A description of the observed behavior (crash, sanitizer report, etc.) and,
  if you have it, a minimal reproducer.

A sanitizer trace is especially helpful. You can reproduce many memory-safety
issues by building with:

```
make CXXFLAGS="-fsanitize=address,undefined -g -O1" \
     LDFLAGS="-fsanitize=address,undefined"
```

(The flag must be passed at both compile and link time; this project's
`Makefile` links via the implicit rule, so `LDFLAGS` is required as well.)

## What to Expect

As a one-person project, response is best-effort rather than bound to a strict
SLA. Realistically:

- **Acknowledgement**: within about a week.
- **Assessment and fix**: depends on severity and complexity; valid
  memory-safety reports will be prioritized.
- **Disclosure**: please allow the fix to land on `main` before discussing the
  issue publicly. Credit will gladly be given in the commit or advisory unless
  you ask to remain anonymous.

Thank you for helping keep the project safe.
