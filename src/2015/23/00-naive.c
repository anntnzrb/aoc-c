/* 2015 day 23, 00 naive: https://adventofcode.com/2015/day/23
 *
 * approach: parse the six instructions, then interpret until the PC exits
 * cost: time O(n + steps), memory O(n), one input pass
 * trade: direct simulation; loops execute every instruction
 */
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

static Instruction *parse(char *in, size_t *count);
static long long run(const Instruction *code, size_t count);

static Instruction *
parse(char *in, size_t *count)
{
	Instruction *code = NULL, ins;
	char *line, *save, reg;
	size_t cap = 0;
	int end;

	*count = 0;
	for (line = strtok_r(in, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		ins.reg = 0;
		ins.offset = 1;
		end = 0;
		if (sscanf(line, "%3s", ins.op) != 1)
			die("invalid instruction");
		if (strcmp(ins.op, "jmp") == 0) {
			if (sscanf(line + 3, " %lld%n", &ins.offset, &end) !=
			    1)
				die("invalid jump");
		} else {
			if (strcmp(ins.op, "jie") == 0 ||
			    strcmp(ins.op, "jio") == 0) {
				if (sscanf(line + 3, " %c, %lld%n", &reg,
				           &ins.offset, &end) != 2)
					die("invalid conditional jump");
			} else if (strcmp(ins.op, "hlf") == 0 ||
			           strcmp(ins.op, "tpl") == 0 ||
			           strcmp(ins.op, "inc") == 0) {
				if (sscanf(line + 3, " %c%n", &reg, &end) != 1)
					die("invalid register instruction");
			} else {
				die("unknown instruction");
			}
			if (reg != 'a' && reg != 'b')
				die("invalid register");
			ins.reg = reg - 'a';
		}
		if (line[3 + end] != '\0')
			die("trailing instruction data");
		if (*count == cap) {
			cap = cap ? cap * 2 : 64;
			code = ereallocarray(code, cap, sizeof(*code));
		}
		code[(*count)++] = ins;
	}
	if (*count > LLONG_MAX)
		die("program too long");
	return code;
}

static long long
run(const Instruction *code, size_t count)
{
	const Instruction *ins;
	long long reg[2] = {0, 0}, pc = 0, step;

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
	if (printf("%lld\n", run(code, count)) < 0)
		die("printf:");
	free(code);
	free(in);
	return EXIT_SUCCESS;
}
