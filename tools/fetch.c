/* fetch: download an Advent of Code puzzle input into inputs/YEAR/dayNN.txt
 *
 * usage: fetch year day
 *
 * The session cookie comes from, in order:
 *   1. AOC_SESSION in the environment
 *   2. AOC_SESSION in ./.env
 *   3. the newest adventofcode.com "session" cookie in a Firefox profile
 * Inputs are cached: an existing file is never re-downloaded.
 * curl(1) speaks TLS and sqlite3(1) reads the cookie database.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aoc.h"

#define ENVKEY "AOC_SESSION="
#define QUERY                                                                 \
	"SELECT expiry, value FROM moz_cookies "                              \
	"WHERE host LIKE '%adventofcode.com' AND name = 'session';"

AOC_PRINTF(1, 2) static char *smprintf(const char *fmt, ...);
AOC_NORETURN static void child(char *const argv[], const int ip[2],
                               const int op[2]);
static pid_t spawn(char *const argv[], int *in, int *out);
static int reap(pid_t pid);
static char *capture(char *const argv[]);
static int feed(char *const argv[], const char *data);
static char *dotenv(void);
static char *firefox(void);
static char *session(void);
static void mkdirp(const char *path);

static char *
smprintf(const char *fmt, ...)
{
	va_list ap;
	char *s;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (n < 0)
		die("fetch: vsnprintf:");
	s = ecalloc((size_t)n + 1, 1);
	va_start(ap, fmt);
	n = vsnprintf(s, (size_t)n + 1, fmt, ap);
	va_end(ap);
	if (n < 0)
		die("fetch: vsnprintf:");
	return s;
}

/* in the forked child: wire stdio to the pipes, then become argv */
static void
child(char *const argv[], const int ip[2], const int op[2])
{
	int null, i;

	if ((null = open("/dev/null", O_RDWR)) < 0)
		_exit(127);
	if (dup2(ip[0] >= 0 ? ip[0] : null, STDIN_FILENO) < 0 ||
	    dup2(op[1] >= 0 ? op[1] : STDERR_FILENO, STDOUT_FILENO) < 0 ||
	    (op[1] >= 0 && dup2(null, STDERR_FILENO) < 0))
		_exit(127);
	/* a leftover write end would keep stdin from ever reaching EOF */
	for (i = 0; i < 2; i++) {
		if (ip[i] > STDERR_FILENO)
			(void)close(ip[i]);
		if (op[i] > STDERR_FILENO)
			(void)close(op[i]);
	}
	if (null > STDERR_FILENO)
		(void)close(null);
	/* fetch ignores SIGPIPE; the child should not inherit that */
	(void)signal(SIGPIPE, SIG_DFL);
	execvp(argv[0], argv);
	_exit(127);
}

/* run argv with its stdin and/or stdout on pipes; a NULL out sends stdout
 * to stderr, a NULL in reads /dev/null; stderr goes to /dev/null when
 * stdout is captured, since then only the output matters */
static pid_t
spawn(char *const argv[], int *in, int *out)
{
	int ip[2] = {-1, -1}, op[2] = {-1, -1};
	pid_t pid;

	if ((in && pipe(ip) < 0) || (out && pipe(op) < 0))
		die("fetch: pipe:");
	if ((pid = fork()) < 0)
		die("fetch: fork:");
	if (pid == 0)
		child(argv, ip, op);
	if (in) {
		(void)close(ip[0]);
		*in = ip[1];
	}
	if (out) {
		(void)close(op[1]);
		*out = op[0];
	}
	return pid;
}

/* wait for pid; nonzero unless it exited 0 */
static int
reap(pid_t pid)
{
	int st;

	while (waitpid(pid, &st, 0) < 0)
		if (errno != EINTR)
			die("fetch: waitpid:");
	return !WIFEXITED(st) || WEXITSTATUS(st) != 0;
}

/* stdout of argv; empty if it fails, like a shell's || true */
static char *
capture(char *const argv[])
{
	FILE *fp;
	char *s;
	size_t len;
	int fd;
	pid_t pid;

	pid = spawn(argv, NULL, &fd);
	if (!(fp = fdopen(fd, "r")))
		die("fetch: fdopen:");
	s = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("fetch: fclose:");
	if (reap(pid))
		s[0] = '\0';
	return s;
}

/* run argv with data on its stdin; nonzero if it fails */
static int
feed(char *const argv[], const char *data)
{
	size_t n = strlen(data);
	ssize_t w;
	int fd;
	pid_t pid;

	pid = spawn(argv, &fd, NULL);
	while (n > 0) {
		if ((w = write(fd, data, n)) < 0) {
			if (errno == EINTR)
				continue;
			/* EPIPE: the child died early; reap() reports it */
			break;
		}
		data += w;
		n -= (size_t)w;
	}
	if (close(fd) < 0)
		die("fetch: close:");
	return reap(pid);
}

/* AOC_SESSION from ./.env: KEY=VALUE lines, parsed, never sourced */
static char *
dotenv(void)
{
	FILE *fp;
	char *buf, *line, *val, *s = NULL;
	size_t len;

	if (!(fp = fopen(".env", "r"))) {
		if (errno == ENOENT)
			return NULL;
		die("fetch: fopen .env:");
	}
	buf = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("fetch: fclose .env:");
	for (line = strtok(buf, "\n"); line && !s; line = strtok(NULL, "\n")) {
		if (strncmp(line, ENVKEY, sizeof(ENVKEY) - 1) != 0)
			continue;
		val = line + sizeof(ENVKEY) - 1;
		if ((len = strcspn(val, "\r")) > 0)
			s = smprintf("%.*s", (int)len, val);
	}
	free(buf);
	return s;
}

/* newest adventofcode.com session cookie across Firefox profiles */
static char *
firefox(void)
{
	static const char *const dirs[] = {
	    "Library/Application Support/Firefox/Profiles", /* macOS */
	    ".mozilla/firefox",                             /* Linux */
	    ".config/mozilla/firefox",                      /* Linux, XDG */
	};
	char prog[] = "sqlite3", query[] = QUERY;
	char *argv[4], *home, *pat, *uri, *out, *line, *bar, *best = NULL;
	long long exp, bestexp = 0;
	glob_t g;
	size_t i, j, len;

	if (!(home = getenv("HOME")) || !*home)
		return NULL;
	for (i = 0; i < LEN(dirs); i++) {
		pat = smprintf("%s/%s/*/cookies.sqlite", home, dirs[i]);
		if (glob(pat, 0, NULL, &g) != 0) {
			free(pat);
			continue;
		}
		for (j = 0; j < g.gl_pathc; j++) {
			/* immutable=1 reads the db while Firefox holds its
			 * lock */
			uri = smprintf("file:%s?immutable=1", g.gl_pathv[j]);
			argv[0] = prog;
			argv[1] = uri;
			argv[2] = query;
			argv[3] = NULL;
			out = capture(argv);
			for (line = strtok(out, "\n"); line;
			     line = strtok(NULL, "\n")) {
				errno = 0;
				exp = strtoll(line, &bar, 10);
				if (errno || bar == line || *bar != '|' ||
				    (best && exp <= bestexp))
					continue;
				free(best);
				best = smprintf("%s", bar + 1);
				bestexp = exp;
			}
			free(out);
			free(uri);
		}
		globfree(&g);
		free(pat);
	}
	if (best && (len = strlen(best)) > 0 && best[len - 1] == '\r')
		best[len - 1] = '\0';
	return best;
}

/* the session cookie, checked so it cannot smuggle extra headers */
static char *
session(void)
{
	const char *env;
	char *s;
	size_t i;

	if ((env = getenv("AOC_SESSION")) && *env)
		s = smprintf("%s", env);
	else if (!(s = dotenv()) && !(s = firefox()))
		die("fetch: no session: set AOC_SESSION in .env or log in "
		    "with Firefox");
	for (i = 0; s[i]; i++)
		if (!isxdigit((unsigned char)s[i]))
			die("fetch: session cookie is not hex");
	if (i == 0)
		die("fetch: empty session cookie");
	return s;
}

static void
mkdirp(const char *path)
{
	if (mkdir(path, 0777) < 0 && errno != EEXIST)
		die("fetch: mkdir %s:", path);
}

int
main(int argc, char *argv[])
{
	struct stat st;
	char prog[] = "curl", fl[] = "-fsS", af[] = "-A",
	     agent[] = "aoc-c tools/fetch", hf[] = "-H", stdinhdr[] = "@-",
	     of[] = "-o";
	char *cargv[10], *dir, *out, *tmp, *url, *hdr, *s;
	const char *err;
	int year, day;

	if (argc != 3)
		die("usage: fetch year day");
	if ((err = puzzleday(argv[1], argv[2], &year, &day)))
		die("fetch: %s %s: %s", argv[1], argv[2], err);

	dir = smprintf("inputs/%d", year);
	out = smprintf("%s/day%02d.txt", dir, day);
	if (stat(out, &st) == 0 && st.st_size > 0) {
		if (printf("%s already exists\n", out) < 0)
			die("fetch: printf:");
		free(out);
		free(dir);
		return EXIT_SUCCESS;
	}

	/* a dead curl must surface as EPIPE, not kill us mid-write */
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		die("fetch: signal:");

	s = session();
	mkdirp("inputs");
	mkdirp(dir);
	tmp = smprintf("%s.tmp", out);
	url = smprintf("https://adventofcode.com/%d/day/%d/input", year, day);
	hdr = smprintf("Cookie: session=%s\n", s);

	/* the cookie goes through stdin so it never shows up in ps(1);
	 * AoC asks automated tools to identify themselves in the
	 * User-Agent */
	cargv[0] = prog;
	cargv[1] = fl;
	cargv[2] = af;
	cargv[3] = agent;
	cargv[4] = hf;
	cargv[5] = stdinhdr;
	cargv[6] = of;
	cargv[7] = tmp;
	cargv[8] = url;
	cargv[9] = NULL;
	if (feed(cargv, hdr)) {
		(void)unlink(tmp);
		die("fetch: download failed (expired session or day not "
		    "unlocked?)");
	}
	if (rename(tmp, out) < 0) {
		(void)unlink(tmp);
		die("fetch: rename %s:", tmp);
	}
	if (printf("%s\n", out) < 0)
		die("fetch: printf:");

	free(hdr);
	free(url);
	free(tmp);
	free(s);
	free(out);
	free(dir);
	return EXIT_SUCCESS;
}
