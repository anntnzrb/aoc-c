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

static char *request(char *url, char *path, char *post);
static char *dotenv(void);
static char *firefox(void);
static char *session(void);
static char *plaintext(const char *p, const char *end, char *out);
static char *articles(char *html);
static void download(int year, int day, int input);

/* Close the header pipe before reading the body: curl waits for EOF. */
static char *
request(char *url, char *path, char *post)
{
	char prog[] = "curl", fl[] = "-fsS", af[] = "-A",
	     agent[] = "aoc-c tools/web", hf[] = "-H", stdinhdr[] = "@-",
	     of[] = "-o", df[] = "--data";
	char *argv[10], *s, *hdr, *data, *body;
	FILE *fp;
	size_t n, len;
	ssize_t w;
	int p[2], q[2], i = 6;
	pid_t pid;

	s = session();
	data = hdr = smprintf("Cookie: session=%s\n", s);
	free(s);
	argv[0] = prog;
	argv[1] = fl;
	argv[2] = af;
	argv[3] = agent;
	argv[4] = hf;
	argv[5] = stdinhdr;
	if (path || post) {
		argv[i++] = path ? of : df;
		argv[i++] = path ? path : post;
	}
	argv[i++] = url;
	argv[i] = NULL;
	xpipe(p);
	xpipe(q);
	pid = spawn(argv, p[0], q[1], -1);
	if (close(p[0]) < 0 || close(q[1]) < 0)
		die("web: close:");
	n = strlen(hdr);
	while (n > 0) {
		if ((w = write(p[1], data, n)) < 0) {
			if (errno == EINTR)
				continue;
			/* EPIPE: the child died early; reap() reports it. */
			break;
		}
		data += w;
		n -= (size_t)w;
	}
	free(hdr);
	if (close(p[1]) < 0)
		die("web: close:");
	if (!(fp = fdopen(q[0], "r")))
		die("web: fdopen:");
	body = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("web: fclose:");
	if (reap(pid)) {
		if (path)
			(void)unlink(path);
		die("web: request failed (expired session or day not "
		    "unlocked?)");
	}
	return body;
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
		die("web: fopen .env:");
	}
	buf = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("web: fclose .env:");
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
		die("web: no session: set AOC_SESSION in .env or log in "
		    "with Firefox");
	for (i = 0; s[i]; i++)
		if (!isxdigit((unsigned char)s[i]))
			die("web: session cookie is not hex");
	if (i == 0)
		die("web: empty session cookie");
	return s;
}

static char *
plaintext(const char *p, const char *end, char *out)
{
	static const char *const breaks[] = {
	    "</p>", "</li>", "</pre>", "</h2>", "<br>", "<br ", "<br/",
	};
	static const char *const entities[] = {
	    "&lt;", "&gt;", "&amp;", "&quot;", "&#39;",
	};
	static const char decoded[] = "<>&\"'";
	const char *tag;
	size_t i, n;

	while (p < end) {
		if (*p == '<') {
			if (!(tag = strchr(p, '>')) || tag >= end)
				die("web: incomplete tag");
			for (i = 0; i < LEN(breaks); i++)
				if (!strncmp(p, breaks[i],
				             strlen(breaks[i]))) {
					*out++ = '\n';
					break;
				}
			p = tag + 1;
			continue;
		}
		for (i = 0; i < LEN(entities); i++) {
			n = strlen(entities[i]);
			if (!strncmp(p, entities[i], n)) {
				*out++ = decoded[i];
				p += n;
				break;
			}
		}
		if (i == LEN(entities))
			*out++ = *p++;
	}
	return out;
}

/* Convert in place, keeping only articles so past answers cannot leak. */
static char *
articles(char *html)
{
	char *p = html, *out = html, *end;

	while ((p = strstr(p, "<article"))) {
		p += sizeof("<article") - 1;
		if (*p != '>' && !isspace((unsigned char)*p))
			continue;
		if (!(p = strchr(p, '>')))
			die("web: incomplete article");
		p++;
		if (!(end = strstr(p, "</article>")))
			die("web: incomplete article");
		out = plaintext(p, end, out);
		if (out > html && out[-1] != '\n')
			*out++ = '\n';
		p = end + sizeof("</article>") - 1;
	}
	*out = '\0';
	return html;
}

static void
download(int year, int day, int input)
{
	struct stat st;
	FILE *fp;
	char *dir, *out, *tmp, *url, *body;

	dir = smprintf("%s/%d", input ? "inputs" : "puzzles", year);
	out = input ? smprintf("%s/day%02d.txt", dir, day)
	            : smprintf("%s/%02d.txt", dir, day);
	if (input && stat(out, &st) == 0 && st.st_size > 0) {
		if (printf("%s already exists\n", out) < 0)
			die("web: printf:");
		free(out);
		free(dir);
		return;
	}
	url = smprintf("https://adventofcode.com/%d/day/%d%s", year, day,
	               input ? "/input" : "");
	tmp = smprintf("%s.tmp", out);
	mkdirp(dir);
	body = request(url, input ? tmp : NULL, NULL);
	if (!input) {
		articles(body);
		if (!*body)
			die("web: no puzzle articles");
		if (!(fp = fopen(tmp, "w")))
			die("web: fopen %s:", tmp);
		if (fputs(body, fp) == EOF || fclose(fp) == EOF) {
			(void)unlink(tmp);
			die("web: write %s:", tmp);
		}
	}
	if (rename(tmp, out) < 0) {
		(void)unlink(tmp);
		die("web: rename %s:", tmp);
	}
	if (printf("%s\n", out) < 0)
		die("web: printf:");
	free(body);
	free(url);
	free(tmp);
	free(out);
	free(dir);
}

int
main(int argc, char *argv[])
{
	char *url, *body, *post;
	const char *err;
	int year, day, submit, rc = 2;

	if (argc < 4 ||
	    (strcmp(argv[1], "input") != 0 && strcmp(argv[1], "puzzle") != 0 &&
	     strcmp(argv[1], "submit") != 0))
		die("usage: web input|puzzle year day | "
		    "web submit year day level answer");
	submit = strcmp(argv[1], "submit") == 0;
	if (argc != (submit ? 6 : 4))
		die("web: wrong number of arguments");
	if ((err = puzzleday(argv[2], argv[3], &year, &day)))
		die("web: %s %s: %s", argv[2], argv[3], err);
	/* A dead curl must surface as EPIPE, not kill us mid-write. */
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		die("web: signal:");
	if (!submit) {
		download(year, day, strcmp(argv[1], "input") == 0);
		return EXIT_SUCCESS;
	}
	if (strcmp(argv[4], "1") != 0 && strcmp(argv[4], "2") != 0)
		die("web: level must be 1 or 2");
	if (!*argv[5] ||
	    strspn(argv[5], "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	                    "abcdefghijklmnopqrstuvwxyz,-") != strlen(argv[5]))
		die("web: answer must match [0-9A-Za-z,-]+");
	post = smprintf("level=%s&answer=%s", argv[4], argv[5]);
	url = smprintf("https://adventofcode.com/%d/day/%d/answer", year, day);
	body = articles(request(url, NULL, post));
	if (fputs(body, stdout) == EOF)
		die("web: fputs:");
	if (strstr(body, "That's the right answer"))
		rc = 0;
	else if (strstr(body, "That's not the right answer"))
		rc = 1;
	free(body);
	free(post);
	free(url);
	return rc;
}
