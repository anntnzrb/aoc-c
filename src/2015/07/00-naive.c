/* 2015 day 07, 00 naive: https://adventofcode.com/2015/day/7
 *
 * approach: sweep the circuit until ready gates propagate a signal to a;
 * resolve wire operands by searching the instruction list, then reset the
 * circuit with b fixed to the first answer and sweep again.
 * cost: time O(n^3), memory O(n), one parse and at most 2n circuit sweeps
 * trade: direct simulation; repeated sweeps and linear wire searches
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	char *left, *right, *op, *out;
	unsigned int value;
	int ready;
} Gate;

static Gate parse(char *line);
static int operandvalue(const char *operand, const Gate *gates, size_t n,
                        unsigned int *value);
static int propagate(Gate *gate, const Gate *gates, size_t n);
static unsigned int evaluate(Gate *gates, size_t n);
static unsigned int solve(char *in, unsigned int *second);

static Gate
parse(char *line)
{
	Gate gate = {0};
	char *words[5], *word, *save;
	size_t n = 0;

	for (word = strtok_r(line, " \t\r", &save);
	     word != NULL && n < LEN(words);
	     word = strtok_r(NULL, " \t\r", &save)) {
		words[n++] = word;
	}
	if (word != NULL)
		die("too many instruction tokens");
	if (n < 3 || strcmp(words[n - 2], "->") != 0)
		die("invalid instruction");
	gate.out = words[n - 1];
	if (n == 3) {
		gate.left = words[0];
	} else if (n == 4 && strcmp(words[0], "NOT") == 0) {
		gate.left = words[1];
		gate.op = words[0];
	} else if (n == 5) {
		gate.left = words[0];
		gate.op = words[1];
		gate.right = words[2];
	} else {
		die("invalid gate");
	}
	return gate;
}

static int
operandvalue(const char *operand, const Gate *gates, size_t n,
             unsigned int *value)
{
	char *end;
	unsigned long literal;
	size_t i;

	if (operand[0] >= '0' && operand[0] <= '9') {
		literal = strtoul(operand, &end, 10);
		if (*end != '\0' || literal > 65535)
			die("invalid signal");
		*value = (unsigned int)literal;
		return 1;
	}
	for (i = 0; i < n; i++) {
		if (strcmp(operand, gates[i].out) == 0) {
			*value = gates[i].value;
			return gates[i].ready;
		}
	}
	return 0;
}

static int
propagate(Gate *gate, const Gate *gates, size_t n)
{
	unsigned int left, right = 0;

	if (gate->ready || !operandvalue(gate->left, gates, n, &left))
		return 0;
	if (gate->right && !operandvalue(gate->right, gates, n, &right))
		return 0;
	if (!gate->op)
		gate->value = left;
	else if (strcmp(gate->op, "NOT") == 0)
		gate->value = left ^ 65535U;
	else if (strcmp(gate->op, "AND") == 0)
		gate->value = left & right;
	else if (strcmp(gate->op, "OR") == 0)
		gate->value = left | right;
	else if (strcmp(gate->op, "LSHIFT") == 0)
		gate->value = right < 16 ? (left << right) & 65535U : 0;
	else if (strcmp(gate->op, "RSHIFT") == 0)
		gate->value = right < 16 ? left >> right : 0;
	else
		die("unknown gate: %s", gate->op);
	gate->ready = 1;
	return 1;
}

static unsigned int
evaluate(Gate *gates, size_t n)
{
	Gate *gate;
	size_t i;
	int progress;

	if (n == 0)
		return 0;
	do {
		progress = 0;
		for (i = 0; i < n; i++) {
			gate = &gates[i];
			if (!propagate(gate, gates, n))
				continue;
			progress = 1;
			if (strcmp(gate->out, "a") == 0)
				return gate->value;
		}
	} while (progress);
	die("wire a has no signal");
}

static unsigned int
solve(char *in, unsigned int *second)
{
	Gate *gates = NULL;
	char *line, *save;
	unsigned int first;
	size_t n = 0, cap = 0, i;

	for (line = strtok_r(in, "\n", &save); line != NULL;
	     line = strtok_r(NULL, "\n", &save)) {
		if (n == cap) {
			cap = cap ? cap * 2 : 16;
			gates = ereallocarray(gates, cap, sizeof(*gates));
		}
		gates[n++] = parse(line);
	}
	first = evaluate(gates, n);
	for (i = 0; i < n; i++) {
		gates[i].ready = strcmp(gates[i].out, "b") == 0;
		gates[i].value = gates[i].ready ? first : 0;
	}
	*second = evaluate(gates, n);
	free(gates);
	return first;
}

int
main(void)
{
	char *in;
	size_t len;
	unsigned int first, second;

	in = slurp(stdin, &len);
	first = solve(in, &second);
	free(in);
	if (printf("%u\n%u\n", first, second) < 0)
		die("printf:");
	return EXIT_SUCCESS;
}
