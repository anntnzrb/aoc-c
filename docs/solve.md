# Solve brief

The contract for a worker that solves one day in its own git worktree. The orchestrator gives YEAR, DAY and MAIN (the main checkout's path), then verifies and merges. Read `AGENTS.md` first; this brief adds only what a worker needs. Never edit this file.

## Fences

- **No Outside Solutions** (`AGENTS.md`): reason from the statement only, never search for or read solutions, never use a remembered answer.
- Write only `src/YEAR/DD/` and `tests/YEAR/DD/` (fixtures only, no scratch files). If lib, tools, the template or the justfile need a change, stop and report it instead; a day-local helper is fine.
- Never edit `AGENTS.md`: put lessons in the report. Never read `.env`.
- Do not load generic Advent of Code or competitive-programming skills: `AGENTS.md` and this brief are the only source of truth for this repo.

## Fast lane: answers first

Speed matters here; 00 is the most direct thing that works.

1. Copy the gitignored inputs from MAIN: `inputs/YEAR/dayDD.txt` and `puzzles/YEAR/DD.txt`.
2. `just new DAY`; write the part-1 examples as `tests/YEAR/DD/p1-*.in|out` (one-line `.out`), plus one combined case when the answer is a sum.
3. **00-naive part 1**: `just test`, `just run DAY`, commit, then submit it yourself (below).
4. **00-naive part 2**: `p2-*` fixtures (two-line `.out`), `just check`, commit, submit, then **stop and report both answers**.

**Submitting** runs from MAIN, where the session lives: `cd MAIN && nix develop -c just submit DAY LEVEL ANSWER`. Exit 0 (right): after part 1, run `just puzzle DAY` in MAIN, copy `puzzles/YEAR/DD.txt` into the worktree and go on with part 2. Exit 1 (wrong): stop and report; never resubmit a guess, lockouts escalate. Exit 2: "too recently" means wait the stated seconds and retry (other workers share the account); "already complete" means the day was solved before, so go on.

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
