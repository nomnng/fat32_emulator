#ifndef FAT_MANAGER_H
#define FAT_MANAGER_H

#include "types.h"

bool load_fat_from_file(char *filepath);
void print_directory_files(char *path);

#endif