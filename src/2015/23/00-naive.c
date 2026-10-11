/* 2015 day 23, 00 naive: https://adventofcode.com/2015/day/23
 *
 * approach: parse once, interpret with a initially zero then one
 * cost: time O(n + steps), memory O(n), one input pass
 * trade: direct simulation; loops execute every instruction
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

typedef struct {
	char op[4];
	int reg;
	long long offset;
} Instruction;

static long long offset(const char *text);
static Instruction instruction(const char *line);
static Instruction *parse(char *in, size_t *count);
static long long run(const Instruction *code, size_t count, long long a);

static long long
offset(const char *text)
{
	char *end;
	long long value;

	errno = 0;
	value = strtoll(text, &end, 10);
	if (errno || end == text || *end || (*text != '+' && *text != '-'))
		die("invalid jump offset");
	return value;
}

static Instruction
instruction(const char *line)
{
	Instruction ins;
	const char *arg;

	if (strlen(line) < 5 || line[3] != ' ')
		die("invalid instruction");
	memcpy(ins.op, line, 3);
	ins.op[3] = '\0';
	ins.reg = 0;
	ins.offset = 1;
	arg = line + 4;
	if (strcmp(ins.op, "jmp") == 0) {
		ins.offset = offset(arg);
		return ins;
	}
	if (*arg != 'a' && *arg != 'b')
		die("invalid register");
	ins.reg = *arg - 'a';
	if (strcmp(ins.op, "jie") == 0 || strcmp(ins.op, "jio") == 0) {
		if (strlen(arg) < 4 || arg[1] != ',' || arg[2] != ' ')
			die("invalid conditional jump");
		ins.offset = offset(arg + 3);
	} else {
		if (strcmp(ins.op, "hlf") != 0 && strcmp(ins.op, "tpl") != 0 &&
		    strcmp(ins.op, "inc") != 0)
			die("unknown instruction");
		if (arg[1])
			die("trailing instruction data");
	}
	return ins;
}

static Instruction *
parse(char *in, size_t *count)
{
	Instruction *code = NULL;
	const char *line;
	char *save;
	size_t cap = 0;

	*count = 0;
	for (line = strtok_r(in, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		if (*count == cap) {
			cap = cap ? cap * 2 : 64;
			code = ereallocarray(code, cap, sizeof(*code));
		}
		code[(*count)++] = instruction(line);
	}
	if (*count > LLONG_MAX)
		die("program too long");
	return code;
}

static long long
run(const Instruction *code, size_t count, long long a)
{
	const Instruction *ins;
	long long reg[2] = {a, 0}, pc = 0, step;

	while (pc >= 0 && pc < (long long)count) {
		ins = &code[pc];
		step = 1;
		if (strcmp(ins->op, "hlf") == 0)
			reg[ins->reg] /= 2;
		else if (strcmp(ins->op, "tpl") == 0)
			reg[ins->reg] *= 3;
		else if (strcmp(ins->op, "inc") == 0)
			reg[ins->reg]++;
		else if (strcmp(ins->op, "jmp") == 0 ||
		         (strcmp(ins->op, "jie") == 0 &&
		          reg[ins->reg] % 2 == 0) ||
		         (strcmp(ins->op, "jio") == 0 && reg[ins->reg] == 1))
			step = ins->offset;
		pc += step;
	}
	return reg[1];
}

int
main(void)
{
	Instruction *code;
	char *in;
	size_t len, count;

	in = slurp(stdin, &len);
	code = parse(in, &count);
	if (printf("%lld\n%lld\n", run(code, count, 0), run(code, count, 1)) <
	    0)
		die("printf:");
	free(code);
	free(in);
	return EXIT_SUCCESS;
}
