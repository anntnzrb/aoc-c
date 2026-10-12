/* util.c: process and filesystem helpers shared by the tools in tools/
 *
 * Children are started with posix_spawnp, not fork + dup2: the kernel-side
 * file actions leave no child code to get wrong, and GCC's -fanalyzer
 * reports a forked child's dup2'd fds as leaks. Every fd the tools open
 * is close-on-exec, so a child only ever holds the three it was given;
 * a stray pipe write end would otherwise keep its stdin from reaching EOF.
 */
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aoc.h"
#include "util.h"

/* POSIX declares it but no header must; the headers here do not */
extern char **environ;

char *
smprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	int n = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (n < 0)
		die("vsnprintf:");
	char *s = ecalloc((size_t)n + 1, 1);
	va_start(ap, fmt);
	n = vsnprintf(s, (size_t)n + 1, fmt, ap);
	va_end(ap);
	if (n < 0)
		die("vsnprintf:");
	return s;
}

size_t
xglob(const char *pat, glob_t *g)
{
	int rc = glob(pat, 0, NULL, g);
	if (rc == GLOB_NOMATCH) {
		g->gl_pathc = 0;
		g->gl_pathv = NULL;
	} else if (rc != 0) {
		die("glob %s failed", pat);
	}
	return g->gl_pathc;
}

void
mkdirp(const char *path)
{
	char *s = smprintf("%s", path);
	for (char *p = s + 1; *p; p++) {
		if (*p != '/')
			continue;
		*p = '\0';
		if (mkdir(s, 0777) < 0 && errno != EEXIST)
			die("mkdir %s:", s);
		*p = '/';
	}
	if (mkdir(s, 0777) < 0 && errno != EEXIST)
		die("mkdir %s:", s);
	free(s);
}

int
xopen(const char *path, int flags)
{
	int fd = open(path, flags | O_CLOEXEC, 0666);
	if (fd < 0)
		die("open %s:", path);
	return fd;
}

void
xpipe(int fd[2])
{
	if (pipe(fd) < 0 || fcntl(fd[0], F_SETFD, FD_CLOEXEC) < 0 ||
	    fcntl(fd[1], F_SETFD, FD_CLOEXEC) < 0)
		die("pipe:");
}

pid_t
spawn(char *const argv[], int fd0, int fd1, int fd2)
{
	posix_spawn_file_actions_t fa;
	posix_spawnattr_t sa;
	sigset_t def;

	/* dup2 clears close-on-exec on the target, so exactly 0-2 survive;
	 * callers may ignore SIGPIPE, which a child must not inherit */
	if ((errno = posix_spawn_file_actions_init(&fa)) ||
	    (errno = posix_spawnattr_init(&sa)) ||
	    (fd0 >= 0 &&
	     (errno = posix_spawn_file_actions_adddup2(&fa, fd0, 0))) ||
	    (fd1 >= 0 &&
	     (errno = posix_spawn_file_actions_adddup2(&fa, fd1, 1))) ||
	    (fd2 >= 0 &&
	     (errno = posix_spawn_file_actions_adddup2(&fa, fd2, 2))) ||
	    sigemptyset(&def) < 0 || sigaddset(&def, SIGPIPE) < 0 ||
	    (errno = posix_spawnattr_setsigdefault(&sa, &def)) ||
	    (errno = posix_spawnattr_setflags(&sa, POSIX_SPAWN_SETSIGDEF)))
		die("posix_spawn setup:");
	pid_t pid;
	if (posix_spawnp(&pid, argv[0], &fa, &sa, argv, environ) != 0)
		pid = -1;
	(void)posix_spawn_file_actions_destroy(&fa);
	(void)posix_spawnattr_destroy(&sa);
	return pid;
}

bool
reap(pid_t pid)
{
	if (pid < 0)
		return true;
	int st;
	while (waitpid(pid, &st, 0) < 0)
		if (errno != EINTR)
			die("waitpid:");
	return !WIFEXITED(st) || WEXITSTATUS(st) != 0;
}

char *
capture(char *const argv[], int fd0, int which, bool *failed)
{
	int p[2];
	xpipe(p);
	int null = xopen("/dev/null", O_RDWR);
	pid_t pid = spawn(argv, fd0 >= 0 ? fd0 : null,
	                  which == 1 ? p[1] : null, which == 2 ? p[1] : null);
	/* the parent's copy of the write end must go before reading */
	if (close(p[1]) < 0 || close(null) < 0)
		die("close:");
	FILE *fp = fdopen(p[0], "r");
	if (!fp)
		die("fdopen:");
	size_t len;
	char *s = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("fclose:");
	*failed = reap(pid);
	return s;
}
