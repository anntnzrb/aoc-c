/* aoc.h: shared helpers for the Advent of Code solutions */
#ifndef AOC_H
#define AOC_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* clang 21's ValistChecker models only __builtin_va_start, not C23's
 * __builtin_c23_va_start; remove when the pinned analyzer learns it. */
#ifdef __clang_analyzer__
#undef va_start
#define va_start(ap, ...) __builtin_va_start(ap, __VA_ARGS__)
#endif

#define LEN(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* print to stderr and exit(1); a trailing ':' in fmt appends strerror */
[[noreturn, gnu::format(printf, 1, 2)]] void die(const char *fmt, ...);

/* allocators that die instead of returning NULL */
[[nodiscard]] void *ecalloc(size_t nmemb, size_t size);
[[nodiscard]] void *ereallocarray(void *p, size_t nmemb, size_t size);

/* read all of fp into a NUL-terminated buffer; length (sans NUL) in *len */
[[nodiscard]] char *slurp(FILE *fp, size_t *len);

/* intern a name, dying if new names exceed capacity or 63 characters */
[[nodiscard]] size_t nameid(char names[][64], size_t *count, size_t capacity,
                            const char *name);

/* advance index order lexicographically; false at the last permutation */
[[nodiscard]] bool nextperm(size_t *order, size_t n);

/* parse a puzzle year and day ("4" or "04") into *year and *day;
 * NULL on success, else why they are not a valid AoC puzzle */
[[nodiscard]] const char *puzzleday(const char *ys, const char *ds, int *year,
                                    int *day);

#endif /* AOC_H */
