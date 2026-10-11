set shell := ["sh", "-eu", "-c"]
set dotenv-load

cc := env_var_or_default("CC", "cc")

# Puzzle year for new/fetch/build/release/run; override with
# `just year=2025 run 01`, AOC_YEAR in the environment, or AOC_YEAR in .env
year := env_var_or_default("AOC_YEAR", "2015")

# C99 + POSIX.1-2008, no extensions (suckless.org/coding_style)
std := "-std=c99 -pedantic-errors -D_POSIX_C_SOURCE=200809L"

# Every clang warning as an error, minus the ones that only make noise.
# -Wno-implicit-void-ptr-cast: C++-compat noise; casting malloc is wrong in C.
warn := "-Weverything -Werror -Wno-padded -Wno-unsafe-buffer-usage -Wno-pre-c11-compat -Wno-implicit-void-ptr-cast -Wdeclaration-after-statement"

# ASan's runtime hangs at startup on macOS 27 (Apple and nixpkgs clang alike),
# so Darwin gets UBSan only; Linux gets ASan too
san := if os() == "macos" { "-fsanitize=undefined,integer" } else { "-fsanitize=address,undefined,integer" }

debug := "-g -O1 -fno-omit-frame-pointer " + san + " -fno-sanitize-recover=all"
# glibc's fortified stdio wrappers are marked unused, so every printf trips
# -Wused-but-marked-unused; nothing in our code can avoid it
fortify := if os() == "linux" { "-Wno-used-but-marked-unused" } else { "" }
release := "-O2 -D_FORTIFY_SOURCE=3 -fstack-protector-strong " + fortify

cflags := std + " " + warn + " -Iinclude"
lib := "lib/aoc.c"

default:
    @just --list

# Build bin/tools/{day,fetch} when their sources changed
[private]
tools:
    #!/bin/sh
    set -eu
    mkdir -p bin/tools
    for t in day fetch; do
        b=bin/tools/$t
        [ "$b" -nt "tools/$t.c" ] && [ "$b" -nt {{lib}} ] &&
            [ "$b" -nt include/aoc.h ] && continue
        {{cc}} {{cflags}} {{debug}} -o "$b" "tools/$t.c" {{lib}}
    done

# Scaffold src/YEAR/dayNN.c and tests/YEAR/dayNN/ from the template
new day: tools
    #!/bin/sh
    set -eu
    n=$(bin/tools/day "{{year}}" "{{day}}")
    d=$(printf '%02d' "$n")
    src="src/{{year}}/day$d.c"
    [ ! -e "$src" ] || { echo "$src exists" >&2; exit 1; }
    mkdir -p "src/{{year}}" "tests/{{year}}/day$d"
    sed -e "s/YEAR/{{year}}/g; s/DAYN/$n/; s/DAY/$d/" templates/day.c >"$src"
    echo "$src"

# Download inputs/YEAR/dayNN.txt (session from .env or Firefox)
fetch day: tools
    bin/tools/fetch "{{year}}" "{{day}}"

# Sanitized debug build of src/YEAR/dayNN.c into bin/YEAR/dayNN
build day: tools
    #!/bin/sh
    set -eu
    d=$(printf '%02d' "$(bin/tools/day "{{year}}" "{{day}}")")
    mkdir -p "bin/{{year}}"
    set -x
    {{cc}} {{cflags}} {{debug}} -o "bin/{{year}}/day$d" "src/{{year}}/day$d.c" {{lib}}

# Optimised, hardened build into bin/YEAR/dayNN.rel
release day: tools
    #!/bin/sh
    set -eu
    d=$(printf '%02d' "$(bin/tools/day "{{year}}" "{{day}}")")
    mkdir -p "bin/{{year}}"
    set -x
    {{cc}} {{cflags}} {{release}} -o "bin/{{year}}/day$d.rel" "src/{{year}}/day$d.c" {{lib}}

# Run a day on its input (default: inputs/YEAR/dayNN.txt)
run day input="": (build day)
    #!/bin/sh
    set -eu
    d=$(printf '%02d' "$(bin/tools/day "{{year}}" "{{day}}")")
    in="{{input}}"
    "./bin/{{year}}/day$d" <"${in:-inputs/{{year}}/day$d.txt}"

# Library unit tests plus every tests/YEAR/dayNN/*.in example, all years
test:
    #!/bin/sh
    set -eu
    mkdir -p bin
    {{cc}} {{cflags}} {{debug}} -o bin/test-lib tests/lib.c {{lib}}
    ./bin/test-lib
    fail=0
    for dir in tests/*/day*/; do
        [ -d "$dir" ] || continue
        y=${dir#tests/}; y=${y%%/*}
        d=${dir#tests/"$y"/day}; d=${d%/}
        just year="$y" build "$d" >/dev/null 2>&1 || {
            echo "FAIL $dir: build"; fail=1; continue; }
        for f in "$dir"*.in; do
            [ -f "$f" ] || continue
            want=${f%.in}.out
            # an .out may list only part 1; compare that many lines
            got=$("./bin/$y/day$d" <"$f" | head -n "$(wc -l <"$want")")
            if [ "$got" = "$(cat "$want")" ]; then
                echo "ok   $f"
            else
                echo "FAIL $f: want '$(cat "$want")' got '$got'"
                fail=1
            fi
        done
    done
    exit $fail

# Format all C sources in place
fmt:
    #!/bin/sh
    set -eu
    files="include/*.h lib/*.c tests/*.c templates/*.c tools/*.c"
    ls src/*/*.c >/dev/null 2>&1 && files="$files src/*/*.c"
    clang-format -i $files

# Static analysis: format drift, clang-tidy, clang static analyzer
lint:
    #!/bin/sh
    set -eu
    files="lib/*.c tests/*.c templates/*.c tools/*.c"
    ls src/*/*.c >/dev/null 2>&1 && files="$files src/*/*.c"
    clang-format --dry-run --Werror include/*.h $files
    clang-tidy --quiet $files -- {{cflags}}
    # nix's cc wrapper injects -L flags that --analyze never uses
    {{cc}} --analyze -Xanalyzer -analyzer-werror -Wno-unused-command-line-argument {{cflags}} $files
    rm -f *.plist

# Full local gate: lint, tests, debug + release build of every day, all years
check: lint tools test
    #!/bin/sh
    set -eu
    for f in src/*/day*.c; do
        [ -f "$f" ] || continue
        y=${f#src/}; y=${y%%/*}
        d=${f#src/"$y"/day}; d=${d%.c}
        just year="$y" build "$d" >/dev/null 2>&1
        just year="$y" release "$d" >/dev/null 2>&1
        echo "built $y/day$d"
    done

clean:
    rm -rf bin *.plist
