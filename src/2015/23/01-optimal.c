/* 2015 day 23, 01 optimal: https://adventofcode.com/2015/day/23
 *
 * approach: decode opcodes and fuse an exactly matched counting loop
 * cost: time O(n + steps), memory O(n), one input and one code pass
 * bound: Omega(n) to read; fused loop uses O(k) recurrence transitions.
 * No closed form is derived for k; general programs retain O(steps).
 * Odd transitions include the mandatory following halving.
 * rejected: just bench 23 50, Linux work: string dispatch 280468;
 * enum dispatch 51937; one transition per fused iteration 16528.
 * Winner pairs odd and even transitions: 16242 (16241-16242, n=5).
 */
#include <errno.h>
#include <limits.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aoc.h"

enum { HALF, TRIPLE, INC, JUMP, EVEN, ONE, LOOP };
typedef struct {
	long long offset;
	int op, reg;
} Instruction;

static Instruction
decode(const char *line)
{
	static const char *const names[] = {"hlf", "tpl", "inc",
	                                    "jmp", "jie", "jio"};
	if (strlen(line) < 5 || line[3] != ' ')
		die("invalid instruction");
	size_t op = 0;
	for (; op < LEN(names); op++)
		if (strncmp(line, names[op], 3) == 0)
			break;
	if (op == LEN(names))
		die("unknown instruction");
	Instruction ins = {1, (int)op, 0};
	const char *arg = line + 4;
	if (ins.op != JUMP) {
		if (*arg != 'a' && *arg != 'b')
			die("invalid register");
		ins.reg = *arg++ - 'a';
	}
	if (ins.op < JUMP) {
		if (*arg)
			die("trailing instruction data");
		return ins;
	}
	if (ins.op != JUMP) {
		if (strlen(arg) < 3 || arg[0] != ',' || arg[1] != ' ')
			die("invalid conditional jump");
		arg += 2;
	}
	errno = 0;
	char *end;
	ins.offset = strtoll(arg, &end, 10);
	if (errno || end == arg || *end || (*arg != '+' && *arg != '-'))
		die("invalid jump offset");
	return ins;
}

static void
fuse(Instruction *code, size_t n)
{
	static const Instruction loop[] = {
	    {8, ONE, 0}, {1, INC, 1},  {4, EVEN, 0}, {1, TRIPLE, 0},
	    {1, INC, 0}, {2, JUMP, 0}, {1, HALF, 0}, {-7, JUMP, 0}};
	for (size_t i = 0; i + LEN(loop) <= n; i++) {
		size_t j = 0;
		for (; j < LEN(loop); j++)
			if (code[i + j].op != loop[j].op ||
			    code[i + j].reg != loop[j].reg ||
			    code[i + j].offset != loop[j].offset)
				break;
		if (j == LEN(loop))
			code[i].op = LOOP;
	}
}

static void
countloop(long long reg[2])
{
	while (reg[0] != 1) {
		if (ckd_add(&reg[1], reg[1], 1))
			die("register overflow");
		if (reg[0] % 2) {
			if (ckd_mul(&reg[0], reg[0], 3) ||
			    ckd_add(&reg[0], reg[0], 1) ||
			    ckd_add(&reg[1], reg[1], 1))
				die("register overflow");
		}
		reg[0] /= 2;
	}
}

static long long
run(const Instruction *code, size_t n, long long a)
{
	long long reg[2] = {a, 0}, pc = 0;

	while (pc >= 0 && pc < (long long)n) {
		const Instruction *ins = &code[pc];
		long long step = 1;
		switch (ins->op) {
		case HALF:
			reg[ins->reg] /= 2;
			break;
		case TRIPLE:
			if (ckd_mul(&reg[ins->reg], reg[ins->reg], 3))
				die("register overflow");
			break;
		case INC:
			if (ckd_add(&reg[ins->reg], reg[ins->reg], 1))
				die("register overflow");
			break;
		case JUMP:
			step = ins->offset;
			break;
		case EVEN:
			if (reg[ins->reg] % 2 == 0)
				step = ins->offset;
			break;
		case ONE:
			if (reg[ins->reg] == 1)
				step = ins->offset;
			break;
		case LOOP:
			countloop(reg);
			step = 8;
			break;
		default:
			die("invalid opcode");
		}
		if (ckd_add(&pc, pc, step))
			die("jump overflow");
	}
	return reg[1];
}

int
main(void)
{
	size_t len;
	char *in = slurp(stdin, &len), *save;
	Instruction *code = NULL;
	size_t n = 0, cap = 0;
	for (const char *line = strtok_r(in, "\n", &save); line;
	     line = strtok_r(NULL, "\n", &save)) {
		if (n == cap) {
			if (!cap)
				cap = 64;
			else if (ckd_mul(&cap, cap, 2))
				die("program too long");
			code = ereallocarray(code, cap, sizeof(*code));
		}
		code[n++] = decode(line);
	}
	if (n > LLONG_MAX)
		die("program too long");
	fuse(code, n);
	if (printf("%lld\n%lld\n", run(code, n, 0), run(code, n, 1)) < 0)
		die("printf:");
	free(code);
	free(in);
	return EXIT_SUCCESS;
}
