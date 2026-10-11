# Solve brief

The contract for a worker that solves one day end-to-end in its own git worktree. The orchestrator gives YEAR, DAY and MAIN (the main checkout's path), then verifies and merges. Read `AGENTS.md` first; this brief adds only what a worker needs. Never edit this file.

## Fences

- **No Outside Solutions** (`AGENTS.md`): reason from the statement only, never search for or read solutions, never use a remembered answer.
- Write only `src/YEAR/DD/` and `tests/YEAR/DD/`. If lib, tools, the template or the justfile need a change, stop and report it instead.
- Never edit `AGENTS.md`: put lessons in the report. Never submit answers, never read `.env`.
- Do not load generic Advent of Code or competitive-programming skills: `AGENTS.md` and this brief are the only source of truth for this repo.

## Steps

1. Copy the gitignored inputs from MAIN: `inputs/YEAR/dayDD.txt` and `puzzles/YEAR/DD.txt` (the statement, both parts when unlocked).
2. `just new DAY`, then write the statement's examples as `tests/YEAR/DD/p1-*.in|out` and `p2-*.in|out`, plus one combined case when the answer is a sum. A part-1 `.out` has one line; a part-2 `.out` has both.
3. **00-naive part 1**: the most direct thing that works. `just check`, cross-check the answer with an independent quick script (awk, for example), commit.
4. **00-naive part 2**, same loop, commit.
5. **01-optimal**: argue the lower bound, try at least two candidates, keep the least `work` from `just bench DAY 50`, record the losers with their numbers in `rejected:`, commit.
6. Optional `02+` explorations when one shows a distinct idea, one commit each.

Commit with `uv run --script ~/.claude/skills/autommit/scripts/cli.py` and nothing staged (it then stages everything). Check each subject against Commit Guidelines.

## Report

- Both answers, and how the cross-check confirmed them.
- `just bench DAY 50` lines for every solution.
- 01's bound in two lines, and the candidates with their work numbers.
- The commits, oldest first.
- Lessons for `AGENTS.md`, if any.
