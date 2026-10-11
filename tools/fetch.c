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
#include <glob.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "aoc.h"
#include "util.h"

#define ENVKEY "AOC_SESSION="
#define QUERY                                                                 \
	"SELECT expiry, value FROM moz_cookies "                              \
	"WHERE host LIKE '%adventofcode.com' AND name = 'session';"

static int feed(char *const argv[], const char *data);
static char *dotenv(void);
static char *firefox(void);
static char *session(void);

/* run argv with data on its stdin and its stdout on our stderr; nonzero
 * if it fails */
static int
feed(char *const argv[], const char *data)
{
	size_t n = strlen(data);
	ssize_t w;
	int p[2];
	pid_t pid;

	xpipe(p);
	pid = spawn(argv, p[0], 2, -1);
	if (close(p[0]) < 0)
		die("fetch: close:");
	while (n > 0) {
		if ((w = write(p[1], data, n)) < 0) {
			if (errno == EINTR)
				continue;
			/* EPIPE: the child died early; reap() reports it */
			break;
		}
		data += w;
		n -= (size_t)w;
	}
	if (close(p[1]) < 0)
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
	char *argv[4], *home, *pat, *uri, *out, *bar, *best = NULL;
	const char *line;
	long long exp, bestexp = 0;
	glob_t g;
	size_t i, j, len, n;
	int failed;

	if (!(home = getenv("HOME")) || !*home)
		return NULL;
	for (i = 0; i < LEN(dirs); i++) {
		pat = smprintf("%s/%s/*/cookies.sqlite", home, dirs[i]);
		n = xglob(pat, &g);
		for (j = 0; j < n; j++) {
			/* immutable=1 reads the db while Firefox holds its
			 * lock */
			uri = smprintf("file:%s?immutable=1", g.gl_pathv[j]);
			argv[0] = prog;
			argv[1] = uri;
			argv[2] = query;
			argv[3] = NULL;
			/* a missing sqlite3 or unreadable db is just no
			 * cookie, like a shell's || true */
			out = capture(argv, -1, 1, &failed);
			if (failed)
				out[0] = '\0';
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
