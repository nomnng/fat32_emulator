#include "shell.h"
#include "fat.h"

#include <stdio.h>
#include <string.h>

static char s_cwd[1024] = "/"; // current working directory

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
	memcpy(str, arguments_str, arguments_len + 1); // remove command and overwrite it with arguments

	return TRUE;
}

static void s_change_cwd(char *path) {
	if (path[0] == '/') {
		strcpy(s_cwd, path);
	} else {
		int cwd_len = strlen(s_cwd);
		if (s_cwd[cwd_len - 1] != '/') {
			strcat(s_cwd, "/");
		}

		strcat(s_cwd, path);
	}
}

void run_shell() {
	char buffer[1024];
	while (1) {
		printf("%s>", s_cwd);
		fgets(buffer, sizeof(buffer), stdin);

		if (s_check_command("exit", buffer)) {
			return;
		} else if (s_check_command("ls", buffer)) {
			fat_print_current_directory_files();
		} else if (s_check_command("cd", buffer)) {
			s_change_cwd(buffer);
			fat_change_current_directory(buffer);
		}
	}
}
