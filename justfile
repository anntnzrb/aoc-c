set shell := ["sh", "-eu", "-c"]
set dotenv-load

# clang, never plain cc: with gcc in the devshell, cc is gcc on Linux
cc := env_var_or_default("CC", "clang")

# Puzzle year for new/fetch/build/release/run/bench; override with
# `just year=2025 run 1`, AOC_YEAR in the environment, or AOC_YEAR in .env
year := env_var_or_default("AOC_YEAR", "2015")

# Every C file the gates cover; src is src/YEAR/DD/NN-slug.c
csrc := "lib/*.c tests/*.c templates/*.c tools/*.c src/*/*/*.c"
lib := "lib/aoc.c"

# C99 + POSIX.1-2008, no extensions (suckless.org/coding_style)
std := "-std=c99 -pedantic-errors -D_POSIX_C_SOURCE=200809L"

# Every clang warning as an error, minus the ones that only make noise.
# -Wno-implicit-void-ptr-cast: C++-compat noise; casting malloc is wrong in C.
warn := "-Weverything -Werror -Wno-padded -Wno-unsafe-buffer-usage -Wno-pre-c11-compat -Wno-implicit-void-ptr-cast -Wdeclaration-after-statement"
cflags := std + " " + warn + " -Iinclude -Itools"

# ASan's runtime hangs at startup on macOS 27 (Apple and nixpkgs clang
# alike), so Darwin gets UBSan only
[linux]
san := "-fsanitize=address,undefined,integer"
[macos]
san := "-fsanitize=undefined,integer"
debug := "-g -O1 -fno-omit-frame-pointer " + san + " -fno-sanitize-recover=all"

# glibc's fortified stdio wrappers are marked unused, so every printf
# trips -Wused-but-marked-unused; nothing in our code can avoid it
[linux]
fortify := "-Wno-used-but-marked-unused"
[macos]
fortify := ""
release := "-O2 -D_FORTIFY_SOURCE=3 -fstack-protector-strong " + fortify

# GCC as a second strict gate on Linux: warnings clang lacks (-Wlogical-op,
# -Wduplicated-cond, ...) plus -fanalyzer; -O2 and fortify so flow-based
# warnings see optimized code
gccwarn := "-O2 -D_FORTIFY_SOURCE=3 -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wdouble-promotion -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-signedness -Wnull-dereference -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wjump-misses-init -Wcast-qual -Wcast-align=strict -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wold-style-definition -Wredundant-decls -Wnested-externs -Wundef -Wunused-macros -Wvla -Walloca -Wdeclaration-after-statement -Wbad-function-cast -Wswitch-default -Wswitch-enum -Wfloat-equal -Wpointer-arith -Wtrampolines -Wstrict-overflow=2 -Winit-self -Wstack-protector -Warray-bounds=2 -Wshift-overflow=2 -Wstringop-overflow=4 -Wunsafe-loop-optimizations -fanalyzer"

# clang static analyzer: default checkers plus security, unix, nullability,
# valist and portability; the memset_s/Annex K advice is off, as in .clang-tidy
analyzer := "-Xanalyzer -analyzer-werror -Xanalyzer -analyzer-checker=security,unix,nullability,valist,optin.portability -Xanalyzer -analyzer-disable-checker=security.insecureAPI.DeprecatedOrUnsafeBufferHandling"

# cppcheck, every check including inconclusive ones. Suppressed:
#   variableScope: suckless declares variables at the top of the function
#   staticFunction in lib/aoc.c: public API that days call (or will)
cppcheck := "--std=c99 --enable=all --inconclusive --check-level=exhaustive --error-exitcode=1 --inline-suppr --quiet --suppress=missingIncludeSystem --suppress=checkersReport --suppress=variableScope --suppress=staticFunction:lib/aoc.c -D_POSIX_C_SOURCE=200809L -Iinclude -Itools"

default:
    @just --list

# Build bin/tools/* (bootstrap: the only loop that cannot be a C tool)
[private]
tools:
    #!/bin/sh
    set -eu
    mkdir -p bin/tools
    for t in day new web build test bench doc; do
        b=bin/tools/$t
        [ -f "$b" ] && [ -z "$(find tools lib include -newer "$b")" ] && continue
        {{ cc }} {{ cflags }} {{ debug }} -o "$b" "tools/$t.c" tools/util.c {{ lib }}
    done

# Scaffold src/YEAR/DD/00-naive.c and tests/YEAR/DD/ from the template
new day: tools
    bin/tools/new {{ year }} {{ day }}

# Download inputs/YEAR/dayDD.txt (session from .env or Firefox)
fetch day: tools
    bin/tools/fetch {{ year }} {{ day }}

# Sanitized debug builds of a day's solutions into bin/YEAR/DD/
build day: tools
    bin/tools/build bin {{ year }} {{ day }} {{ cc }} {{ cflags }} {{ debug }} {{ lib }}

# Optimized, hardened builds of a day's solutions into bin/rel/YEAR/DD/
release day: tools
    bin/tools/build bin/rel {{ year }} {{ day }} {{ cc }} {{ cflags }} {{ release }} {{ lib }}

# Run one solution (default 00) on its input (default inputs/YEAR/dayDD.txt)
run day solution="00" input="": (build day)
    d=$(bin/tools/day {{ year }} {{ day }}) && in="{{ input }}" && bin/{{ year }}/$d/{{ solution }}-* <"${in:-inputs/{{ year }}/day$d.txt}"

# Cost of every solution of a day on its input; all must match 00's answers
bench day runs="100": (release day)
    d=$(bin/tools/day {{ year }} {{ day }}) && bin/tools/bench {{ runs }} inputs/{{ year }}/day$d.txt bin/rel/{{ year }}/$d/*

# Library unit tests, then every solution against every fixture, all years
test: tools
    bin/tools/build bin all all {{ cc }} {{ cflags }} {{ debug }} {{ lib }} >/dev/null
    {{ cc }} {{ cflags }} {{ debug }} -o bin/test-lib tests/lib.c {{ lib }} && bin/test-lib
    bin/tools/test

# Format everything in place: C, flake.nix, justfile
fmt:
    clang-format -i include/*.h {{ csrc }}
    nixfmt flake.nix
    just --fmt --unstable

# Format drift (C, nix, justfile), clang-tidy, clang analyzer, cppcheck,
# solution headers
lint: tools
    bin/tools/doc
    clang-format --dry-run --Werror include/*.h {{ csrc }}
    nixfmt --check flake.nix
    just --fmt --check --unstable >/dev/null
    clang-tidy --quiet {{ csrc }} -- {{ cflags }}
    # nix's cc wrapper injects -L flags that --analyze never uses
    {{ cc }} --analyze {{ analyzer }} -Wno-unused-command-line-argument {{ cflags }} {{ csrc }} && rm -f *.plist
    cppcheck {{ cppcheck }} {{ csrc }}

# GCC strict compile + -fanalyzer over every C file (objects discarded)
[linux]
gcc:
    gcc {{ std }} {{ gccwarn }} -Iinclude -Itools -c {{ csrc }} && rm -f *.o

# GCC on macOS is clang under another name: nothing to add
[macos]
gcc:
    @echo "gcc: skipped on macOS"

# Full gate: lint, GCC, tests, release builds of every solution
check: lint gcc test
    bin/tools/build bin/rel all all {{ cc }} {{ cflags }} {{ release }} {{ lib }} >/dev/null

clean:
    rm -rf bin *.plist *.o
