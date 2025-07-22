#ifndef DIRECTORY_BROWSER_H
#define DIRECTORY_BROWSER_H

#include "types.h"

#define MAX_FILENAME_LEN 255

typedef struct {
	char filename[MAX_FILENAME_LEN + 1];
	u32 file_size;
	u32 first_cluster;
	bool is_directory;
} file_info;

int count_directory_files(void *ptr, int size);
char** list_directory_files(void *ptr, int size, int *out_file_count);
bool next_cluster_file(void *ptr, u32 size, u32 *out_current_entry_index, file_info *out_file_info);
bool find_file_in_directory(void *ptr, u32 size, file_info *out_file_info, char* name);

#endif