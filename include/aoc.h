/* aoc.h: shared helpers for the Advent of Code solutions */
#ifndef AOC_H
#define AOC_H

#include <stddef.h>
#include <stdio.h>

#if defined(__GNUC__) || defined(__clang__)
#define AOC_NORETURN __attribute__((noreturn))
#define AOC_PRINTF(fmt, args) __attribute__((format(printf, fmt, args)))
#else
#define AOC_NORETURN
#define AOC_PRINTF(fmt, args)
#endif

#define LEN(a) (sizeof(a) / sizeof((a)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* print to stderr and exit(1); a trailing ':' in fmt appends strerror */
AOC_NORETURN AOC_PRINTF(1, 2) void die(const char *fmt, ...);

/* allocators that die instead of returning NULL */
void *ecalloc(size_t nmemb, size_t size);
void *ereallocarray(void *p, size_t nmemb, size_t size);

/* read all of fp into a NUL-terminated buffer; length (sans NUL) in *len */
char *slurp(FILE *fp, size_t *len);

/* intern a name, dying if new names exceed capacity or 63 characters */
size_t nameid(char names[][64], size_t *count, size_t capacity,
              const char *name);

/* advance index order lexicographically; return 0 at the last permutation */
int nextperm(size_t *order, size_t n);

/* parse a puzzle year and day ("4" or "04") into *year and *day;
 * NULL on success, else why they are not a valid AoC puzzle */
const char *puzzleday(const char *ys, const char *ds, int *year, int *day);

#endif /* AOC_H */
