#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat.h"
#include "shell.h"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		puts("No input file specified!");
		return 1;
	}

	char *fat_filename = argv[1];

	if (!fat_load_from_file(fat_filename)) {
		return 1;
	}

	run_shell();

	return 0;
}
