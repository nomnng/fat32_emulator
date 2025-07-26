#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "types.h"

#define MAX_FILENAME_LEN 255
#define SFN_LEN 11
#define NEW_DIRECTORY_ENTRIES_SIZE 64

typedef struct {
	char filename[MAX_FILENAME_LEN + 1];
	u32 file_size;
	u32 first_cluster;
	bool is_directory;
} file_info;

int directory_count_files(void *ptr, int size);
bool directory_next_file(void *ptr, u32 size, u32 *out_current_entry_index, file_info *out_file_info);
bool directory_find_file(void *ptr, u32 size, file_info *out_file_info, char* name);
u32 directory_calculate_dir_entry_size(char *directory_name);
void* directory_find_free_entry(void *ptr, u32 size);
void directory_generate_dir_entry(void *buffer, char *directory_name, char *directory_sfn, u32 first_cluster);
void directory_generate_sfn(void *ptr, u32 size, char* name, char* out_sfn);
void directory_generate_new_folder_dir_entries(void *buffer, u32 current_cluster, u32 parent_cluster);

#endif