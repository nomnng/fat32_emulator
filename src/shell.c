#include "shell.h"
#include "types.h"
#include "fat_manager.h"

#include <stdio.h>
#include <string.h>

static char s_cwd[260] = "/"; // current working directory

static bool s_check_command(char* command, char* str) {
	int i;
	for (i = 0; command[i]; i++) {
		if (str[i] != command[i]) {
			return FALSE;
		}
	}

	if (str[i] != '\n' && str[i] != '\0' && str[i] != ' ') {
		return FALSE;
	}

	char *new_line = strchr(str, '\n');
	if (new_line) {
		// cut string if there's more then one line
		new_line[0] = 0;
	}

	if (str[i] == 0) {
		// no arguments after command
		str[0] = 0;
		return TRUE;
	}

	char *arguments_str = str + i + 1;
	int arguments_len = strlen(arguments_str);
	memcpy(str, arguments_str, arguments_len + 1); // copy string including NULL terminating character

	return TRUE;
}

static void s_change_cwd(char *path) {
	strcpy(s_cwd, path);
}

void run_shell() {
	char buffer[1024];
	while (1) {
		printf("%s>", s_cwd);
		fgets(buffer, sizeof(buffer), stdin);

		if (s_check_command("exit", buffer)) {
			return;
		} else if (s_check_command("ls", buffer)) {
			print_directory_files(s_cwd);
		} else if (s_check_command("cd", buffer)) {
			s_change_cwd(buffer);
		} else if (s_check_command("", buffer)) {
			
		} else if (s_check_command("", buffer)) {
			
		} else if (s_check_command("", buffer)) {
			
		}
	}
}
