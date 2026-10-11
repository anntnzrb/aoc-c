# Solve brief

The contract for a worker that solves one day in its own git worktree. The orchestrator gives YEAR, DAY and MAIN (the main checkout's path), submits answers, verifies and merges. Read `AGENTS.md` first; this brief adds only what a worker needs. Never edit this file.

## Fences

- **No Outside Solutions** (`AGENTS.md`): reason from the statement only, never search for or read solutions, never use a remembered answer.
- Write only `src/YEAR/DD/` and `tests/YEAR/DD/` (fixtures only, no scratch files). If lib, tools, the template or the justfile need a change, stop and report it instead; a day-local helper is fine.
- Never edit `AGENTS.md`: put lessons in the report. Never submit answers, never read `.env`.
- Do not load generic Advent of Code or competitive-programming skills: `AGENTS.md` and this brief are the only source of truth for this repo.

## Fast lane: answers first

Speed matters here; 00 is the most direct thing that works.

1. Copy the gitignored inputs from MAIN: `inputs/YEAR/dayDD.txt` and `puzzles/YEAR/DD.txt`.
2. `just new DAY`; write the part-1 examples as `tests/YEAR/DD/p1-*.in|out` (one-line `.out`), plus one combined case when the answer is a sum.
3. **00-naive part 1**: `just test`, `just run DAY`, commit, then **stop and report the answer**. The orchestrator submits, refreshes `puzzles/YEAR/DD.txt` and messages you.
4. **00-naive part 2**: `p2-*` fixtures (two-line `.out`), `just check`, commit, then **stop and report the answer**.

## Slow lane: optimization, on request

5. **01-optimal**: argue the lower bound, try at least two candidates, keep the least `work` from `just bench DAY 50`, record the losers with their numbers in `rejected:`. `just check`, commit.
6. Optional `02+` explorations when one shows a distinct idea, one commit each.

Commit with `uv run --script ~/.claude/skills/autommit/scripts/cli.py` and nothing staged (it then stages everything). Check each subject against Commit Guidelines.

## Report

Keep each report short:
- The answer(s), and any edge case the input or statement hid.
- After 01: `just bench DAY 50` lines, the bound in two lines, the candidates with their work numbers.
- The commits created.
- Lessons for `AGENTS.md`, and anything that belongs in `lib/` because a later day would reuse it.
