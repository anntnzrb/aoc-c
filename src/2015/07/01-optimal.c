/* 2015 day 07, 01 optimal: https://adventofcode.com/2015/day/7
 *
 * approach: scan tokens directly and intern names in a hash table; evaluate
 * a's dependencies with a stack and record their completion order; override
 * b and replay that order for part 2 without recursion or a table reset.
 * cost: expected time O(L), memory O(L), one parse, one walk, one replay
 * bound: reading L input bytes is necessary: any instruction can affect a;
 * each reachable gate has at most two inputs and is evaluated once per part.
 * rejected: strtok_r tokens with stack/order replay: work 433038
 * (433038-433039); direct tokens: 249559 (249558-249560). Recursive memo
 * twice: 460119 (460119-460120); recursive order replay: 430691
 * (430690-430692), both fail misc-no-recursion. Naive: 205944872
 * (205944871-205944874). bench 7 50, Linux CPU 0, five paired counters.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

#define LITERAL (SIZE_MAX / 2 + 1)

typedef enum { UNSET, COPY, NOT, AND, OR, LEFT, RIGHT } Op;
typedef struct {
	char *name;
	size_t left, right;
	unsigned int value;
	Op op;
	int state;
} Gate;
typedef struct {
	Gate *gates;
	size_t *order, cap, count;
} Circuit;

static size_t intern(Circuit *c, char *name);
static size_t operand(Circuit *c, char *word);
static char *token(char **cursor);
static void parse(Circuit *c, char *line);
static unsigned int apply(Op op, unsigned int left, unsigned int right);
static unsigned int cached(const Circuit *c, size_t id);
static size_t dependency(const Circuit *c, const Gate *g);
static unsigned int evaluate(Circuit *c, size_t id);

static size_t
intern(Circuit *c, char *name)
{
	const char *p;
	unsigned int hash = 0;
	size_t id;

	for (p = name; *p; p++) {
		if (*p < 'a' || *p > 'z')
			die("invalid wire name");
		hash = ((hash & 0x07ffffffU) << 5) ^ hash ^ (unsigned char)*p;
	}
	id = hash & (c->cap - 1);
	while (c->gates[id].name && strcmp(c->gates[id].name, name) != 0)
		id = (id + 1) & (c->cap - 1);
	c->gates[id].name = name;
	return id;
}

static size_t
operand(Circuit *c, char *word)
{
	char *end;
	unsigned long value;

	if (word[0] < '0' || word[0] > '9')
		return intern(c, word);
	value = strtoul(word, &end, 10);
	if (*end || value > 65535)
		die("invalid signal");
	return LITERAL | value;
}

static char *
token(char **cursor)
{
	char *p = *cursor, *start;

	while (*p == ' ' || *p == '\t' || *p == '\r')
		p++;
	start = p;
	while (*p && *p != ' ' && *p != '\t' && *p != '\r')
		p++;
	if (*p)
		*p++ = '\0';
	*cursor = p;
	return *start ? start : NULL;
}

static void
parse(Circuit *c, char *line)
{
	Gate *g;
	char *words[5], *word;
	size_t n = 0;

	for (word = token(&line); word && n < LEN(words); word = token(&line))
		words[n++] = word;
	if (word || n < 3 || strcmp(words[n - 2], "->") != 0)
		die("invalid instruction");
	g = &c->gates[intern(c, words[n - 1])];
	if (g->op != UNSET)
		die("duplicate wire source");
	g->right = LITERAL;
	if (n == 3) {
		g->op = COPY;
		g->left = operand(c, words[0]);
	} else if (n == 4 && !strcmp(words[0], "NOT")) {
		g->op = NOT;
		g->left = operand(c, words[1]);
	} else if (n == 5) {
		g->left = operand(c, words[0]);
		g->right = operand(c, words[2]);
		if (!strcmp(words[1], "AND"))
			g->op = AND;
		else if (!strcmp(words[1], "OR"))
			g->op = OR;
		else if (!strcmp(words[1], "LSHIFT"))
			g->op = LEFT;
		else if (!strcmp(words[1], "RSHIFT"))
			g->op = RIGHT;
		else
			die("unknown gate");
	} else {
		die("invalid gate");
	}
}

static unsigned int
apply(Op op, unsigned int left, unsigned int right)
{
	switch ((int)op) {
	case COPY:
		return left;
	case NOT:
		return left ^ 65535U;
	case AND:
		return left & right;
	case OR:
		return left | right;
	case LEFT:
		return right < 16 ? (left << right) & 65535U : 0;
	case RIGHT:
		return right < 16 ? left >> right : 0;
	case UNSET:
	default:
		die("wire has no source");
	}
}

static unsigned int
cached(const Circuit *c, size_t id)
{
	return (id & LITERAL) ? (unsigned int)(id & ~LITERAL)
	                      : c->gates[id].value;
}

static size_t
dependency(const Circuit *c, const Gate *g)
{
	if (!(g->left & LITERAL) && c->gates[g->left].state != 2)
		return g->left;
	if (!(g->right & LITERAL) && c->gates[g->right].state != 2)
		return g->right;
	return LITERAL;
}

static unsigned int
evaluate(Circuit *c, size_t id)
{
	Gate *g;
	size_t depth = 1, current, child;

	/* Stack and completed order grow from opposite ends of one array. */
	c->order[c->cap - 1] = id;
	while (depth) {
		current = c->order[c->cap - depth];
		g = &c->gates[current];
		if (g->state == 2) {
			depth--;
			continue;
		}
		if (g->op == UNSET)
			die("wire has no source");
		g->state = 1;
		child = dependency(c, g);
		if (child != LITERAL) {
			if (c->gates[child].state == 1)
				die("cyclic circuit");
			c->order[c->cap - ++depth] = child;
			continue;
		}
		g->value =
		    apply(g->op, cached(c, g->left), cached(c, g->right));
		g->state = 2;
		depth--;
		c->order[c->count++] = current;
	}
	return c->gates[id].value;
}

int
main(void)
{
	Circuit c = {0};
	Gate *g;
	char *in, *line, *save;
	char a[] = "a", b[] = "b";
	size_t len, i, target, override;
	unsigned int first = 0, second = 0;

	in = slurp(stdin, &len);
	if (len) {
		c.cap = 16;
		/* Even the shortest instruction needs six bytes for two names.
		 */
		while (c.cap < (len / 3) + 16)
			c.cap *= 2;
		c.gates = ecalloc(c.cap, sizeof(*c.gates));
		c.order = ecalloc(c.cap, sizeof(*c.order));
		for (line = strtok_r(in, "\n", &save); line;
		     line = strtok_r(NULL, "\n", &save))
			parse(&c, line);
		target = intern(&c, a);
		override = intern(&c, b);
		first = evaluate(&c, target);
		c.gates[override].value = first;
		for (i = 0; i < c.count; i++) {
			if (c.order[i] == override)
				continue;
			g = &c.gates[c.order[i]];
			g->value = apply(g->op, cached(&c, g->left),
			                 cached(&c, g->right));
		}
		second = c.gates[target].value;
	}
	free(c.order);
	free(c.gates);
	free(in);
	if (printf("%u\n%u\n", first, second) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
