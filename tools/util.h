/* util.h: process and filesystem helpers shared by the tools in tools/ */
#ifndef UTIL_H
#define UTIL_H

#include <glob.h>
#include <sys/types.h>

#include "aoc.h"

/* printf into a fresh allocation */
AOC_PRINTF(1, 2) char *smprintf(const char *fmt, ...);

/* glob(3) that dies on error; returns the match count, 0 for no match.
 * globfree(g) is always safe afterwards */
size_t xglob(const char *pat, glob_t *g);

/* mkdir -p */
void mkdirp(const char *path);

/* open with O_CLOEXEC, so only the fds spawn() wires up reach a child */
int xopen(const char *path, int flags);

/* pipe() with both ends close-on-exec */
void xpipe(int fd[2]);

/* start argv (searched in PATH) with fd0/fd1/fd2 as its stdin/stdout/
 * stderr, or -1 to inherit; the child gets SIGPIPE back at its default.
 * -1 if argv cannot start, which reap() reports like a failed exit */
pid_t spawn(char *const argv[], int fd0, int fd1, int fd2);

/* wait for pid; nonzero unless it exited 0 */
int reap(pid_t pid);

/* run argv with stdin from fd0 (-1: /dev/null) and return what it writes
 * to fd `which` (1 or 2); the other stream goes to /dev/null. *failed is
 * set unless it exited 0 */
char *capture(char *const argv[], int fd0, int which, int *failed);

#endif /* UTIL_H */
