#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "types.h"

#define MAX_FILENAME_LEN 255

typedef struct {
	char filename[MAX_FILENAME_LEN + 1];
	u32 file_size;
	u32 first_cluster;
	bool is_directory;
} file_info;

int directory_count_files(void *ptr, int size);
bool directory_next_file(void *ptr, u32 size, u32 *out_current_entry_index, file_info *out_file_info);
bool directory_find_file(void *ptr, u32 size, file_info *out_file_info, char* name);

#endif