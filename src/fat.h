#ifndef FAT_H
#define FAT_H

#include "types.h"

bool fat_load_from_file(char *filepath);
void fat_print_directory_files(char *path);
bool fat_change_current_directory(char *path);
void fat_print_current_directory_files();
void fat_print_file_content(char *filename);

#endif