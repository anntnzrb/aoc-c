# Repository Guidelines

## Project Overview

Advent of Code solutions for **every year** (2015 onward) in pure C99 + POSIX.1-2008, written in [suckless style](https://suckless.org/coding_style/). The repo layout is one repo per language, so other languages get their own repos. The toolchain comes from a Nix flake devshell, and every task runs through `just`. The goal is maximally strict, warning-free C. Never relax the flags to get a build through.

**Everything that can be C is C.** The helper tools (`tools/*.c`) are held to the same flags, lint and sanitizers as the solutions. Only files that cannot be C are exempt: Markdown, Nix (`flake.nix`, `flake.lock`), the `justfile` and the dotfiles. Do not add shell scripts. Logic that outgrows a justfile one-liner belongs in a C tool.

## Architecture & Data Flow

Each day compiles to a standalone binary that acts as a Unix filter:

```
bin/tools/fetch → inputs/YEAR/dayNN.txt → stdin → slurp() → part1(in, len), part2(in, len) → stdout (one answer per line)
```

- Solutions never open files or parse argv. Input always arrives on stdin.
- `lib/aoc.c` is compiled straight into each binary (`cc … src/YEAR/dayNN.c lib/aoc.c -Iinclude`). No static or shared library is built. `lib/` and `include/` are shared by all years.
- Failures are fail-fast: `die()` exits with status 1. Allocators never return NULL.
- `src/` does not exist until the first `just new <day>`.
- **Year selection:** the justfile variable `year` defaults to `AOC_YEAR` (read from the environment or `.env` via `set dotenv-load`), then to `2015`. Override it per call with `just year=2025 run 01`. `test` and `check` ignore it and cover every year.
- **Day ranges:** `puzzleday()` in `lib/aoc.c` validates them. Both tools call it, every recipe goes through `bin/tools/day`, and all of them accept `4` or `04`. Years 2015–2024 have days 1–25, and 2025 onward has days 1–12.
- **Tools:** the private `tools` recipe builds `bin/tools/day` and `bin/tools/fetch` with the sanitized debug flags. It only rebuilds when `tools/*.c`, `lib/aoc.c` or `include/aoc.h` is newer than the binary. `new`, `fetch`, `build`, `release` and `check` depend on it.

## Key Directories

| Path | Purpose |
|---|---|
| `include/aoc.h` | Shared API: `die`, `ecalloc`, `ereallocarray`, `slurp`, `puzzleday`, plus `LEN`/`MIN`/`MAX` |
| `lib/aoc.c` | Implementation of the shared API |
| `src/YEAR/dayNN.c` | One solution per day, two-digit zero-padded (`src/2015/day01.c`) |
| `templates/day.c` | Skeleton for `just new`. Its `YEAR`/`DAY`/`DAYN` placeholders are replaced by `sed` |
| `tests/lib.c` | Unit tests for `lib/` |
| `tests/YEAR/dayNN/*.in`, `*.out` | Puzzle example fixtures |
| `tools/fetch.c` | Input downloader: `bin/tools/fetch YEAR DAY` |
| `tools/day.c` | Year/day validator for recipes: `bin/tools/day YEAR DAY` prints the plain day number |
| `inputs/YEAR/` | Personal puzzle inputs. **Gitignored, not redistributable** |
| `bin/YEAR/`, `bin/tools/` | Build output. Gitignored |

## Development Commands

Run everything inside the devshell (`nix develop`, or prefix commands with `nix develop -c`).

```sh
just new 4                  # src/2015/day04.c + tests/2015/day04/; refuses to overwrite
just year=2025 new 4        # same, for 2025 (or set AOC_YEAR in .env)
just fetch 4                # inputs/2015/day04.txt (cached; never re-downloads)
just run 4                  # sanitized debug build, then ./bin/2015/day04 < inputs/2015/day04.txt
just run 4 tests/2015/day04/a.in   # run against a specific input
just test                   # lib unit tests + every tests/*/dayNN fixture, all years
just fmt                    # clang-format in place
just lint                   # clang-format check → clang-tidy → clang --analyze
just check                  # lint + test + debug and release builds of every day, all years (full gate)
just release 4              # bin/2015/day04.rel: -O2, FORTIFY_SOURCE=3, stack protector
just clean
```

Recipe arguments are day numbers. Every recipe normalizes `4`/`04` through `bin/tools/day`, which rejects days outside the year's range.

## Code Conventions & Common Patterns

**Language**
- C99 with no extensions: `-std=c99 -pedantic-errors -D_POSIX_C_SOURCE=200809L`.
- Warnings: `-Weverything -Werror`. Only four are disabled everywhere (`warn` in the `justfile`): `padded`, `unsafe-buffer-usage`, `pre-c11-compat` and `implicit-void-ptr-cast`. Release builds on Linux also disable `used-but-marked-unused`, because glibc's fortified stdio wrappers trigger it on every `printf` call. Fix the code instead of adding more `-Wno-*` flags.
- String literals are `const char[]` under `-Wwrite-strings`, and `-Wcast-qual` forbids casting the `const` away. An `argv` for `execvp` is therefore built from local `char x[] = "..."` arrays (see `tools/fetch.c`).
- Some glibc struct members are recursive macros (`sa_handler`), which `-Wdisabled-macro-expansion` rejects. Prefer the plain API (`signal`) over that struct.

**suckless rules, enforced by the compiler, clang-format and clang-tidy**
- Declare every variable at the top of its block, before any statement (`-Wdeclaration-after-statement`). No `for (int i = …)`.
- Use `/* */` comments only.
- Indent with tabs (width 8). Lines are at most 79 columns.
- Put the return type on its own line and the opening `{` on its own line for functions:
  ```c
  static long long
  part1(const char *in, size_t len)
  {
  ```
- Mark every file-local function `static`, and declare its prototype at the top of the file in definition order (see `templates/day.c`). This applies to `lib/` and `tools/` too.
- `case` is not indented under `switch`. Comment intentional fallthrough with `/* FALLTHROUGH */`.
- Attach `*` to the name: `char *p`. Use `int`, not `bool`. Use `CamelCase` for typedefs and never the `_t` suffix.
- Assigning inside a condition is idiomatic: `if (!(fp = fopen(p, "r"))) die("fopen %s:", p);`.
- Never cast `malloc`/`calloc` results.

**Error handling**
- `die(fmt, ...)`: if `fmt` ends in `':'`, it appends `strerror(errno)`, e.g. `die("fopen %s:", path)`.
- `ecalloc`/`ereallocarray` die on failure or overflow, so never NULL-check what they return.
- clang-tidy's `cert-err33-c` flags any ignored stdio return value. Check it, or cast it to `(void)` with a comment explaining why (see `die()` in `lib/aoc.c`).
- Use `fseek(fp, 0, SEEK_SET)`, not `rewind()` (flagged by `bugprone-unsafe-functions`).

**Solution shape** (from `templates/day.c`)
- `static long long part1(const char *in, size_t len)` and `part2` work on the NUL-terminated buffer from `slurp(stdin, &len)`. `len` excludes the NUL.
- `main` prints `"%lld\n%lld\n"`, frees the input buffer and returns `EXIT_SUCCESS`.
- `templates/day.c` is linted too, so it must stay warning-free.

## Important Files

- `justfile` is the single source of truth for compiler flags, sanitizers and recipes.
- `flake.nix` defines the devshell using `clangStdenv`, so Linux uses clang too. It ships `clang-tools`, `just`, and `curl` and `sqlite` (which `fetch` runs at runtime). It sets `hardeningDisable = [ "fortify" "fortify3" ]` so the cc wrapper cannot inject `_FORTIFY_SOURCE` into debug builds, which keeps the `justfile` the only source of flags. `flake.lock` pins nixpkgs.
- `.clang-format` encodes the suckless style. Run `just fmt` instead of hand-formatting.
- `.clang-tidy` enables `bugprone-*`, `cert-*`, `clang-analyzer-*`, `misc-*`, `performance-*`, `portability-*` and `readability-*` as errors. The disabled checks are listed with reasons in the file header. `misc-include-cleaner` ignores glibc's `asm-generic/errno*.h`, because otherwise it rejects POSIX errno macros such as `EINTR` even though `<errno.h>` is included.
- `.env.example` documents the optional keys `AOC_SESSION` and `AOC_YEAR`. `AOC_YEAR` is the default year for `just` recipes and falls back to `2015`.
- **`.env` holds a secret session cookie.** Never read, print or commit it.

## Runtime/Tooling Preferences

- Compiler: clang 21 from nixpkgs. Do not switch to gcc, and do not use Apple's `/usr/bin/clang`.
- **Developed on macOS and Linux; both must pass `just check`.** Platform quirks surface only on the platform that has them, so a change counts as verified only on the platform where it was run.
- **Sanitizers differ by OS.** On macOS 27, ASan hangs at startup with both nixpkgs and Apple clang, so macOS debug builds use `-fsanitize=undefined,integer` and Linux adds `address` (with LeakSanitizer, so tools must free what they allocate). All builds use `-fno-sanitize-recover=all`, so any UB aborts the program.
- `just lint` passes `-Wno-unused-command-line-argument` to `clang --analyze` because the nix cc wrapper injects `-L` flags. Keep that flag.
- `tools/fetch.c` is plain POSIX C. It runs `curl` (TLS) and `sqlite3` (the cookie DB) with `fork`/`execvp` over pipes and links no extra libraries:
  - A cached input (non-empty `inputs/YEAR/dayNN.txt`) exits 0 before any session lookup.
  - Session lookup order: `$AOC_SESSION` from the environment, then the first non-empty `AOC_SESSION=` line in `.env`, then the newest-expiring adventofcode.com cookie across Firefox profiles (macOS `~/Library/Application Support/Firefox/Profiles`, Linux `~/.mozilla/firefox` and `~/.config/mozilla/firefox`), read with `immutable=1`.
  - `.env` is parsed, never executed. A trailing `\r` is stripped.
  - The session must be hex. Anything else (CR/LF, `;`) is rejected so it cannot inject headers.
  - The cookie goes to curl over stdin (`-H @-`) so it stays out of `ps`. `SIGPIPE` is ignored so a dead curl reports a failure instead of killing `fetch`, and children get `SIG_DFL` back before `exec`.
  - Writes are atomic: curl writes `dayNN.txt.tmp`, `rename()` moves it into place, and the tmp file is unlinked on failure.

## Testing & QA

- No test framework: `tests/lib.c` uses `<assert.h>` and builds into `bin/test-lib` with the sanitized debug flags. Never compile it with `-DNDEBUG`. It covers `slurp` and `puzzleday`.
- Day fixtures: `tests/YEAR/dayNN/<name>.in` is fed to `bin/YEAR/dayNN`, and stdout is compared with `<name>.out`. They are copied from the puzzle text, so they are gitignored (`/tests/2*/`), like inputs; the AoC FAQ asks public repos not to include puzzle text or inputs. A day that fails to build reports `FAIL <dir>: build`.
  - Only the first `wc -l < .out` lines are compared, so a one-line `.out` tests part 1 alone while part 2 is unfinished.
  - **Every `.out` must end with a newline**, otherwise `wc -l` returns 0 and the comparison is empty.
  - A mismatch prints `FAIL <file>: want '…' got '…'`, the run continues, and the final exit status is non-zero.
- Workflow per day: `just new N`, add the puzzle examples as fixtures, `just test` until green, then `just run N`, then `just check` before committing.
- `just check` must pass before work counts as done. It builds the tools even when `src/` is empty, but it never executes them, so changes to `tools/` also need a manual run of `bin/tools/day` and of `bin/tools/fetch` against a real or stubbed `curl`. There is no CI yet.

## Commit Guidelines

`git log` should show what was solved and how.

- Subject: `<scope>: <imperative summary>`, lowercase, no period, ≤72 chars.
- Scopes: `YYYY/DD` (`src/YYYY/dayDD.c`), `lib` (header, `lib/aoc.c`, `tests/lib.c`), `tools/<name>`, `template`, `build` (`justfile`, flake, `.clang-*`), `docs`, `repo` (`.gitignore`, `.env.example`).
- One commit per puzzle part: `2015/01: solve part 1 by summing paren steps`, then `2015/01: solve part 2 …`. Split by hunk if both are pending.
- Never mix two days, or a day with shared code. Shared code a day needs is committed first, in its own scoped commit with its tests.
- Body: approach and complexity. Never include answers, input or puzzle text.

## No Outside Solutions

The point of this repo is to watch agents solve AoC on their own. This rule binds every agent, including any agent work is delegated to. Paste it into every delegated prompt.

- Derive every solution from the puzzle statement. Never search for, fetch, or read AoC solutions, write-ups, megathreads, repos or videos, for any year or day.
- Allowed: adventofcode.com itself (statements, inputs, submitting), C/POSIX/compiler/tool documentation, and general algorithm references in software-engineering terms (Dijkstra, union-find, CRT) as long as they are not tied to a specific puzzle.
- If a remembered answer or solution comes to mind, do not use it. Reason from the statement and show that reasoning in the report.
- When the agent's tooling allows it, turn off web search and network access for the implementer, using per-run options only. Never change the user's persistent configuration.

## Solve Workflow

A living procedure. When a run hits a new problem, fix the step and add a line to the lessons log below.

Trigger: "solve N". The year comes from `AOC_YEAR`, or from `year=YYYY` when it is given.

Roles:
- **Orchestrator** runs the pipeline, checks every result and reports.
- **Implementer** writes the solution. It gets the puzzle statement, the No Outside Solutions rule, the target file, and the goal of a green `just test` and `just lint`.
- **Helpers** handle trivial mechanical work, such as extracting example fixtures from the statement.

Steps:
1. **Fetch:** `just fetch N`. Get the statement from adventofcode.com (`just puzzle N` is planned and will write the gitignored `puzzles/`).
2. **Scaffold:** `just new N`.
3. **Fixtures first:** write the statement's examples to `tests/YEAR/dayNN/pK-M.in` and `.out`. Part 1 fixtures get a one-line `.out`.
4. **Solve:** the implementer writes `part1`. If it fails `just test` twice, the orchestrator solves the part itself.
5. **Answer:** `just run N`. Cross-check against an independent quick script whenever one is cheap to write.
6. **Submit part 1** (`just submit N 1 X` is planned). After a wrong answer, stop and diagnose; never resubmit guesses, because lockouts escalate.
7. **Part 2:** fetch the statement again (part 2 unlocks only after part 1 is accepted), then repeat steps 3–6 with level 2.
8. **Gate:** `just check` must exit 0.
9. **Report:** both answers, a ✓/✗ checklist, and a short note on the approach and its complexity.

Lessons log:
- 2015/01: clang-tidy rejects nested ternaries (`readability-avoid-nested-conditional-operator`). Use a `switch`.
- tools: `readability-function-cognitive-complexity` is enforced. Split long functions early (see `child()` in `tools/fetch.c`).
- Linux: glibc false positives exist (fortified stdio, POSIX errno macros). See Important Files and Language before adding any `-Wno-*`.
