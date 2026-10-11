# Repository Guidelines

This whole file is living documentation. When a solve, a bug or a better idea shows a rule is wrong or missing, update it in the same session.

## Project

Advent of Code, every year from 2015, in C99 + POSIX.1-2008, [suckless style](https://suckless.org/coding_style/). One repo per language. The repo is learning material: it records how an agent reasons from a puzzle to a naive answer, then to an optimal one. The toolchain comes from a Nix flake devshell (`nix develop`), and every task runs through `just`.

**Principles**
- Everything that can be C is C. Only Markdown, Nix, the `justfile` and dotfiles are exempt. No shell scripts; a recipe that needs logic gets a C tool in `tools/`.
- Fewer lines win. DRY, high cohesion, low coupling: shared code goes behind a header (`include/aoc.h` for everything, `tools/util.h` for the tools), and a module's documentation lives in its own header comment.
- Maximal strictness: never relax a flag or silence a check to get green. A `-Wno-*` or suppression needs a written reason next to it, and only for a false positive the code cannot avoid.
- Linux (x86_64) is the primary target and macOS (Apple silicon) must also pass. A change counts as verified only on the platform it ran on.

## Layout

```
src/YYYY/DD/NN-slug.c   solutions: 00-naive, 01-optimal, 02+ explorations
tests/YYYY/DD/*.in|out  puzzle-example fixtures (gitignored)
inputs/YYYY/dayDD.txt   personal inputs (gitignored, never commit)
include/aoc.h lib/aoc.c die, ecalloc, ereallocarray, slurp, puzzleday; linked into every binary
tools/*.c, tools/util.* day, new, web, build, test, bench, doc; see each file's header
templates/day.c         skeleton for `just new` (YEAR, DAY, DAYN placeholders)
bin/YYYY/DD/            sanitized debug builds; bin/rel/YYYY/DD/ release; bin/tools/
```

Every solution is a Unix filter: input on stdin, part 1 then part 2 on stdout, one answer per line, exit 0. Days and years are validated by `puzzleday()`: 2015-2024 have days 1-25, 2025 on have 1-12; `4` and `04` both work.

## Commands

```sh
just new 4            # src/YEAR/04/00-naive.c + tests/YEAR/04/
just fetch 4          # inputs/YEAR/day04.txt (cached)
just puzzle 4         # puzzles/YEAR/04.txt (always refetched, articles only)
just submit 4 1 X     # submit part 1 answer and print the response
just run 4 [NN] [in]  # debug build, run solution NN (default 00)
just test             # lib unit tests + every solution x every fixture, all years
just bench 4 [runs]   # release builds: answers must equal 00's, then wall/cpu/rss/instructions
just fmt              # clang-format, nixfmt, just --fmt
just lint             # format drift, clang-tidy, clang analyzer, cppcheck
just gcc              # GCC -fanalyzer + extra warnings (Linux; no-op on macOS)
just check            # lint + gcc + test + release builds: the gate
```

`year` defaults to `AOC_YEAR` (environment or `.env`), then 2015: `just year=2025 run 1`.

## Toolchain

- `justfile` is the single source of flags. clang 21 is primary (`-Weverything -Werror`, four documented `-Wno-*`). GCC 16 is a second gate on Linux. The devshell's `cc` is GCC on Linux, so recipes use `$CC` (`clang`), never `cc`.
- Debug builds run ASan (Linux only: it hangs on macOS 27), UBSan and integer sanitizers with `-fno-sanitize-recover=all`; LeakSanitizer means tools free what they allocate. MemorySanitizer crashes with nixpkgs glibc and is not used.
- `flake.nix` disables the cc wrapper's fortify hardening, which would otherwise inject `_FORTIFY_SOURCE` into debug builds.
- Platform differences use just's `[linux]`/`[macos]` attributes, never `os()` branches.
- Known glibc false positives, each handled once with a reason: fortified stdio vs `-Wused-but-marked-unused` (release, Linux), include-cleaner wanting `bits/` and `asm-generic/` headers (`.clang-tidy`), `-fanalyzer` flagging `fork` + `dup2` (the tools use `posix_spawnp`).
- `.env` holds the secret session cookie: never read, print or commit it.

## Code

suckless, enforced by the compilers, clang-format, clang-tidy and cppcheck: tabs, 79 columns, `/* */` comments, return type on its own line, variables at the top of the block, file-local functions `static` with prototypes at the top in definition order, `case` not indented, `char *p`, `int` not `bool`, `CamelCase` typedefs, assignment in conditions is fine, never cast `malloc`. `die(fmt)` appends `strerror` when `fmt` ends in `:`, and the `e*` allocators never return NULL. Check every stdio return or cast it to `(void)` with a reason.

Under `-Wwrite-strings` + `-Wcast-qual`, an `argv` for spawning is built from local `char x[] = "..."` arrays. Prefer plain APIs over glibc structs whose members are macros (`signal`, not `sigaction`).

## Solutions

- **00-naive** is written first: the most direct thing that works, competition style. It is the oracle: `test` and `bench` hold every other solution to its answers. The template is its skeleton.
- **01-optimal** is the gold standard: as close to the lower bound as known, ranked by asymptotic cost and passes, then `perf` user-space instructions ("work", startup subtracted), then peak RSS, CPU and wall time. If an exploration beats it, swap them.
- **02+** explorations: two passes, recursion, memoization, tables, branch-free, anything that shows a distinct idea.
- Every file opens with a header: `approach:` (the idea, from what the puzzle asks), `cost:` (time, memory, passes), then `bound:` and `rejected:` for 01 (why nothing beats it; what lost, with `just bench` numbers) or `trade:` for the rest. `just lint` enforces the tags (`tools/doc.c`). Comment only decision points inside the code.

## Commits

`git log` should show what was solved and how.
- Subject: `<scope>: <imperative summary>`, lowercase, no period, ≤72 chars. Scopes: `YYYY/DD`, `lib`, `tools/<name>`, `template`, `build` (justfile, flake, `.clang-*`), `docs`, `repo` (`.gitignore`, `.env.example`).
- One commit per step: 00 part 1, 00 part 2, 01, each exploration. Commit part 1 before writing part 2; a new file's two parts are one hunk and will not split later.
- Shared code a day needs is committed first, in its own scope, with its tests. Never mix two days.
- Body: approach and cost. Never answers, input or puzzle text.

## No Outside Solutions

Binds every agent, including delegates; paste it into every delegated prompt.
- Derive every solution from the puzzle statement. Never search for, fetch or read AoC solutions, write-ups, megathreads, repos or videos.
- Allowed: adventofcode.com itself, C/POSIX/tool documentation, general algorithms (Dijkstra, union-find, CRT) not tied to a puzzle.
- A remembered answer or solution is not used; reason from the statement and show the reasoning.
- Disable the implementer's web search and network access with per-run options when its tooling allows; never change the user's persistent configuration.

## Solve Workflow

Trigger: "solve N". A worker in its own worktree follows `docs/solve.md` end-to-end; the orchestrator verifies (gate, bench, independent cross-check) and merges days in order. Orchestrator runs the pipeline and checks every result; the implementer writes code from the statement, this file and a green `just test` target; helpers do mechanical work such as fixture extraction.

1. `just fetch N`, `just puzzle N`, `just new N`.
2. Write the statement's examples as fixtures; a part-1 `.out` has one line.
3. Implement 00 part 1, `just run N`, cross-check with a quick independent script when cheap, `just submit N 1 ANSWER`. After a wrong answer stop and diagnose; lockouts escalate. `just check`, commit.
4. Part 2 likewise (it unlocks only after part 1), commit.
5. Write 01: argue the bound, `just bench N`, commit. Add explorations when they teach something.
6. Report: answers, ✓/✗, approach, cost vs bound, bench numbers. Implementer failing `just test` twice: the orchestrator takes over.

Lessons:
- autommit commits only the staged snapshot when anything is staged: after `git mv`, run `git add -A` first or the content edits stay behind (2015/02's `web` rename landed as two runs).
- On an account that already solved a day, `just submit` only answers "already complete" and verifies nothing; rely on the 00 oracle plus an independent cross-check (awk, for example).
- `-fsanitize=integer` rejects shifts that discard set bits, even on unsigned values: mask before rotating a hash (2015/03).
- A per-element branch can block vectorization: 2015/01's fused loop with a basement check ran more instructions than two passes. Measure before believing "one pass".
