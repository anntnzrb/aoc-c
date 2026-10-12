/* lib.c: unit tests for lib/aoc.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "aoc.h"

static void
test_slurp_empty(void)
{
	FILE *fp = tmpfile();
	if (!fp)
		die("tmpfile:");
	size_t len;
	char *s = slurp(fp, &len);
	assert(len == 0);
	assert(s[0] == '\0');
	free(s);
	if (fclose(fp) == EOF)
		die("fclose:");
}

/* bigger than several growth steps, so every realloc path runs */
static void
test_slurp_multichunk(void)
{
	FILE *fp = tmpfile();
	if (!fp)
		die("tmpfile:");
	constexpr size_t n = 100000;
	for (size_t i = 0; i < n; i++)
		if (fputc('a' + (int)(i % 26), fp) == EOF)
			die("fputc:");
	if (fseek(fp, 0, SEEK_SET) < 0)
		die("fseek:");

	size_t len;
	char *s = slurp(fp, &len);
	assert(len == n);
	assert(strlen(s) == n);
	for (size_t i = 0; i < n; i++)
		assert(s[i] == 'a' + (int)(i % 26));
	free(s);
	if (fclose(fp) == EOF)
		die("fclose:");
}

static void
test_puzzleday(void)
{
	int y = 0, d = 0;

	assert(!puzzleday("2015", "4", &y, &d) && y == 2015 && d == 4);
	assert(!puzzleday("2015", "08", &y, &d) && d == 8);
	assert(!puzzleday("2024", "25", &y, &d) && d == 25);
	assert(!puzzleday("2025", "12", &y, &d) && y == 2025 && d == 12);
	assert(puzzleday("2025", "13", &y, &d));
	assert(puzzleday("2015", "26", &y, &d));
	assert(puzzleday("2015", "0", &y, &d));
	assert(puzzleday("2014", "1", &y, &d));
	assert(puzzleday("", "1", &y, &d));
	assert(puzzleday("2015", "", &y, &d));
	assert(puzzleday("2015", "-1", &y, &d));
	assert(puzzleday("2015", "4x", &y, &d));
	assert(puzzleday("99999999999", "1", &y, &d));
}

static void
test_nameid(void)
{
	char names[2][64], name[64];
	size_t n = 0;

	assert(nameid(names, &n, LEN(names), "a") == 0 && n == 1);
	memset(name, 'b', sizeof(name) - 1);
	name[63] = '\0';
	assert(nameid(names, &n, LEN(names), name) == 1 && n == 2);
	name[0] = 'c';
	assert(names[1][0] == 'b' && strlen(names[1]) == 63);
	assert(nameid(names, &n, LEN(names), "a") == 0 && n == 2);
	assert(strcmp(names[0], "a") == 0);
}

static void
test_nameid_reject(void)
{
	char names[2][64] = {"a", "b"}, name[65];
	memset(name, 'b', sizeof(name) - 1);
	name[64] = '\0';
	for (size_t i = 0; i < 2; i++) {
		pid_t pid = fork();
		if (pid < 0)
			die("fork:");
		if (!pid) {
			size_t n = 2;
			(void)nameid(names, &n, i ? 3 : 2, i ? name : "c");
			_exit(0);
		}
		int status;
		if (waitpid(pid, &status, 0) < 0)
			die("waitpid:");
		assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
	}
}

static void
test_nextperm(void)
{
	size_t order[] = {0, 1, 2};
	constexpr size_t want[][3] = {
	    {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};

	assert(!nextperm(NULL, 0));
	assert(!nextperm(order, 1) && order[0] == 0);
	assert(nextperm(order, 2) && order[0] == 1 && order[1] == 0);
	assert(!nextperm(order, 2) && order[0] == 1 && order[1] == 0);
	order[0] = 0;
	order[1] = 1;
	for (size_t i = 0; i < LEN(want); i++) {
		assert(nextperm(order, LEN(order)));
		assert(!memcmp(order, want[i], sizeof(order)));
	}
	assert(!nextperm(order, LEN(order)));
	assert(!memcmp(order, want[4], sizeof(order)));
}

static void
test_realloc_overflow(void)
{
	pid_t pid = fork();
	if (pid < 0)
		die("fork:");
	if (!pid) {
		(void)ereallocarray(NULL, SIZE_MAX, 2);
		_exit(0);
	}
	int status;
	if (waitpid(pid, &status, 0) < 0)
		die("waitpid:");
	assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
}

int
main(void)
{
	test_slurp_empty();
	test_slurp_multichunk();
	test_puzzleday();
	test_nameid();
	test_nameid_reject();
	test_nextperm();
	test_realloc_overflow();
	if (puts("lib: ok") == EOF)
		die("puts:");
	return EXIT_SUCCESS;
}
