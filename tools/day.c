/* day: validate a puzzle year and day, print the day as two digits (04)
 *
 * usage: day year day
 */
#include <stdio.h>
#include <stdlib.h>

#include "aoc.h"

int
main(int argc, char *argv[])
{
	const char *err;
	int year, day;

	if (argc != 3)
		die("usage: day year day");
	if ((err = puzzleday(argv[1], argv[2], &year, &day)))
		die("day: %s %s: %s", argv[1], argv[2], err);
	if (printf("%02d\n", day) < 0)
		die("day: printf:");
	return EXIT_SUCCESS;
}
