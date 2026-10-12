/* web: authenticated Advent of Code input, puzzle statements and submissions
 *
 * usage: web input year day | web puzzle year day
 *        web submit year day level answer
 *
 * Session lookup: AOC_SESSION environment, ./.env, newest Firefox cookie.
 * Sessions must be hex; curl receives the cookie on stdin, never its argv.
 * Inputs are cached; statements always refetch and keep only article contents,
 * stripping the page's past answers. Both files are replaced atomically.
 * Submission levels are 1 or 2; answers must match [0-9A-Za-z,-]+.
 * Submit exits 0 for right, 1 for wrong, 2 for cooldown/already complete,
 * and 3 for unknown responses.
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

static constexpr char ENVKEY[] = "AOC_SESSION=";

/* AOC_SESSION from ./.env: KEY=VALUE lines, parsed, never sourced */
static char *
dotenv(void)
{
	FILE *fp = fopen(".env", "r");
	if (!fp) {
		if (errno == ENOENT)
			return NULL;
		die("web: fopen .env:");
	}
	size_t len;
	char *buf = slurp(fp, &len);
	if (fclose(fp) == EOF)
		die("web: fclose .env:");
	char *s = NULL;
	for (char *line = strtok(buf, "\n"); line && !s;
	     line = strtok(NULL, "\n")) {
		if (strncmp(line, ENVKEY, sizeof(ENVKEY) - 1) != 0)
			continue;
		char *val = line + sizeof(ENVKEY) - 1;
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
	char *home = getenv("HOME");
	if (!home || !*home)
		return NULL;
	char prog[] = "sqlite3", query[] =
	                             "SELECT expiry, value FROM moz_cookies "
	                             "WHERE host LIKE '%adventofcode.com' "
	                             "AND name = 'session';";
	char *best = NULL;
	long long bestexp = 0;
	for (size_t i = 0; i < LEN(dirs); i++) {
		char *pat = smprintf("%s/%s/*/cookies.sqlite", home, dirs[i]);
		glob_t g;
		size_t n = xglob(pat, &g);
		for (size_t j = 0; j < n; j++) {
			/* immutable=1 reads the db while Firefox holds its
			 * lock */
			char *uri =
			    smprintf("file:%s?immutable=1", g.gl_pathv[j]);
			char *argv[] = {prog, uri, query, NULL};
			/* a missing sqlite3 or unreadable db is just no
			 * cookie, like a shell's || true */
			bool failed;
			char *out = capture(argv, -1, 1, &failed);
			if (failed)
				out[0] = '\0';
			for (const char *line = strtok(out, "\n"); line;
			     line = strtok(NULL, "\n")) {
				errno = 0;
				char *bar;
				long long exp = strtoll(line, &bar, 10);
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
	size_t len;
	if (best && (len = strlen(best)) > 0 && best[len - 1] == '\r')
		best[len - 1] = '\0';
	return best;
}

/* the session cookie, checked so it cannot smuggle extra headers */
static char *
session(void)
{
	char *s;
	const char *env = getenv("AOC_SESSION");
	if (env && *env)
		s = smprintf("%s", env);
	else if (!(s = dotenv()) && !(s = firefox()))
		die("web: no session: set AOC_SESSION in .env or log in "
		    "with Firefox");
	size_t i = 0;
	for (; s[i]; i++)
		if (!isxdigit((unsigned char)s[i]))
			die("web: session cookie is not hex");
	if (i == 0)
		die("web: empty session cookie");
	return s;
}

/* Close the header pipe before reading the body: curl waits for EOF. */
static char *
request(char *url, char *path, char *post)
{
	char *s = session();
	char *hdr = smprintf("Cookie: session=%s\n", s), *data = hdr;
	free(s);
	char prog[] = "curl", fl[] = "-fsS", af[] = "-A",
	     agent[] = "aoc-c tools/web", hf[] = "-H", stdinhdr[] = "@-",
	     of[] = "-o", df[] = "--data";
	char *argv[10] = {prog, fl, af, agent, hf, stdinhdr};
	char *opt = path ? of : df, *arg = path ? path : post;
	int i = 6;
	if (arg) {
		argv[i++] = opt;
		argv[i++] = arg;
	}
	argv[i++] = url;
	argv[i] = NULL;
	int p[2], q[2];
	xpipe(p);
	xpipe(q);
	pid_t pid = spawn(argv, p[0], q[1], -1);
	if (close(p[0]) < 0 || close(q[1]) < 0)
		die("web: close:");
	size_t n = strlen(hdr);
	while (n > 0) {
		ssize_t w = write(p[1], data, n);
		if (w < 0) {
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
	FILE *fp = fdopen(q[0], "r");
	if (!fp)
		die("web: fdopen:");
	size_t len;
	char *body = slurp(fp, &len);
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

static char *
plaintext(const char *p, const char *end, char *out)
{
	static const char *const breaks[] = {
	    "</p>", "</li>", "</pre>", "</h2>", "<br>", "<br ", "<br/",
	};
	static const char *const entities[] = {
	    "&lt;", "&gt;", "&amp;", "&quot;", "&#39;", "&apos;",
	};
	static constexpr char decoded[] = "<>&\"''";

	while (p < end) {
		if (*p == '<') {
			const char *tag = strchr(p, '>');
			if (!tag || tag >= end)
				die("web: incomplete tag");
			for (size_t i = 0; i < LEN(breaks); i++)
				if (!strncmp(p, breaks[i],
				             strlen(breaks[i]))) {
					*out++ = '\n';
					break;
				}
			p = tag + 1;
			continue;
		}
		size_t i = 0;
		for (; i < LEN(entities); i++) {
			size_t n = strlen(entities[i]);
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
	char *p = html, *out = html;

	while ((p = strstr(p, "<article"))) {
		p += sizeof("<article") - 1;
		if (*p != '>' && !isspace((unsigned char)*p))
			continue;
		if (!(p = strchr(p, '>')))
			die("web: incomplete article");
		p++;
		char *end = strstr(p, "</article>");
		if (!end)
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
download(int year, int day, bool input)
{
	struct stat st;
	char *dir = smprintf("%s/%d", input ? "inputs" : "puzzles", year);
	char *out = input ? smprintf("%s/day%02d.txt", dir, day)
	                  : smprintf("%s/%02d.txt", dir, day);
	if (input && stat(out, &st) == 0 && st.st_size > 0) {
		if (printf("%s already exists\n", out) < 0)
			die("web: printf:");
		free(out);
		free(dir);
		return;
	}
	char *url = smprintf("https://adventofcode.com/%d/day/%d%s", year, day,
	                     input ? "/input" : "");
	char *tmp = smprintf("%s.tmp", out);
	mkdirp(dir);
	char *body = request(url, input ? tmp : NULL, NULL);
	if (!input) {
		articles(body);
		if (!*body)
			die("web: no puzzle articles");
		FILE *fp = fopen(tmp, "w");
		if (!fp)
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
	if (argc < 4 ||
	    (strcmp(argv[1], "input") != 0 && strcmp(argv[1], "puzzle") != 0 &&
	     strcmp(argv[1], "submit") != 0))
		die("usage: web input|puzzle year day | "
		    "web submit year day level answer");
	bool submit = strcmp(argv[1], "submit") == 0;
	if (argc != (submit ? 6 : 4))
		die("web: wrong number of arguments");
	int year, day;
	const char *err = puzzleday(argv[2], argv[3], &year, &day);
	if (err)
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
	char *post = smprintf("level=%s&answer=%s", argv[4], argv[5]);
	char *url =
	    smprintf("https://adventofcode.com/%d/day/%d/answer", year, day);
	char *body = articles(request(url, NULL, post));
	if (fputs(body, stdout) == EOF)
		die("web: fputs:");
	int rc = 3;
	if (strstr(body, "That's the right answer"))
		rc = 0;
	else if (strstr(body, "That's not the right answer"))
		rc = 1;
	else if (strstr(body, "You gave an answer too recently") ||
	         strstr(body, "You don't seem to be solving the right level"))
		rc = 2;
	free(body);
	free(post);
	free(url);
	return rc;
}
