# oulu

Host snapshot behind the rows of `bench/*.csv` whose `host` is `oulu`. Taken 2026-10-11.

## Hardware

- CPU: 13th Gen Intel Core i7-1355U, x86_64, hybrid: 2 performance cores (CPUs 0-3 with hyperthreading) and 8 efficiency cores (CPUs 4-11), 12 logical CPUs
- Clock: 400 MHz to 5.0 GHz, turbo enabled, governor `performance`
- Cache: L1d 352 KiB, L1i 576 KiB, L2 6.5 MiB, L3 12 MiB
- Memory: 23 GiB

## System

- OS: NixOS 26.05, Linux 7.2.8
- `perf_event_paranoid`: 2 (user-space counters allowed)

## Toolchain (devshell)

- nixpkgs: `8edc0c7` (locked 2026-10-08)
- clang 21.1.8 (primary), GCC 16.2.0 (second gate), cppcheck 2.22.0, perf 7.2.9, just 1.58.0, Nix 2.34.8

## Measurement notes

- `just bench` pins to CPU 0, a performance core. CPU 1 is its hyperthread sibling; load there adds noise to wall and CPU time, not to instruction counts.
- Turbo and the `performance` governor make wall time depend on temperature and load; compare runs by `work` first.
