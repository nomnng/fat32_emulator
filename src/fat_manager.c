#include "fat_manager.h"
#include "directory_browser.h"
#include "fat32_reserved_area.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define END_OF_CHAIN_CLUSTER 0x0FFFFFF8
#define CLUSTER_NUMBER_MASK 0x0FFFFFFF

static u32 s_cluster_size = 0; // cluster size in bytes
static u32 s_first_data_sector = 0; // number of first data sector
static FILE *s_fat_file = NULL;
static fat_boot_sector s_boot_sector;
static u32 *s_fat = NULL; // pointer to fat itself

static void *s_cached_cluster_buffer = NULL;
static u32 s_cached_cluster_number = 0xFFFFFFFF;

static void s_read_sectors(u32 offset, u32 count, void *dst_buffer) {
	fseek(s_fat_file, offset * s_boot_sector.sector_size, SEEK_SET);
	fread(dst_buffer, s_boot_sector.sector_size, count, s_fat_file);
}

static void s_read_clusters_into_buffer(u32 offset, u32 count, void *dst_buffer) {
	fseek(s_fat_file, s_first_data_sector * s_boot_sector.sector_size + offset * s_cluster_size, SEEK_SET);
	fread(dst_buffer, s_cluster_size, count, s_fat_file);
}

static u32 s_normalize_cluster_number(u32 raw_cluster_number) {
	return (raw_cluster_number & CLUSTER_NUMBER_MASK) - 2;
}

static void* s_get_cluster(u32 cluster_number) {
	cluster_number = s_normalize_cluster_number(cluster_number);
	if (s_cached_cluster_number != cluster_number) {
		fseek(s_fat_file, s_first_data_sector * s_boot_sector.sector_size + cluster_number * s_cluster_size, SEEK_SET);
		fread(s_cached_cluster_buffer, s_cluster_size, 1, s_fat_file);
		s_cached_cluster_number = cluster_number;
	}

	return s_cached_cluster_buffer;
}


static bool s_is_end_of_chain_cluster(u32 cluster_number) {
	return (cluster_number & CLUSTER_NUMBER_MASK) >= END_OF_CHAIN_CLUSTER;
}

static u32 s_get_next_cluster_number(u32 cluster_number) {
	return s_fat[cluster_number];
}

static bool s_next_directory_file(u32 *out_current_cluster_number, u32 *out_current_entry_index, file_info *fi) {
	void *current_cluster = s_get_cluster(*out_current_cluster_number);

	if (next_cluster_file(current_cluster, s_cluster_size, out_current_entry_index, fi)) {
		return TRUE;
	} else {
		u32 next_cluster = s_get_next_cluster_number(*out_current_cluster_number);
		if (s_is_end_of_chain_cluster(next_cluster)) {
			return FALSE;
		}

		*out_current_entry_index = 0;
		*out_current_cluster_number = next_cluster;
		return s_next_directory_file(out_current_cluster_number, out_current_entry_index, fi);
	}
}

bool load_fat_from_file(char *filepath) {
	s_fat_file = fopen(filepath, "rb+");
	if (!s_fat_file) {
		return FALSE;
	}

	fread(&s_boot_sector, sizeof(s_boot_sector), 1, s_fat_file);
	s_cluster_size = s_boot_sector.sector_size * s_boot_sector.sectors_per_cluster;
	s_first_data_sector = s_boot_sector.reserved_sectors + s_boot_sector.fats * s_boot_sector.fat32_length;

	s_cached_cluster_buffer = malloc(s_cluster_size);

	s_fat = (u32*) malloc(s_boot_sector.sector_size * s_boot_sector.fat32_length);
	s_read_sectors(s_boot_sector.reserved_sectors, s_boot_sector.fat32_length, s_fat);

	return TRUE;
}

void print_directory_files(char *path) {
	u32 current_entry_index = 0;
	u32 current_cluster = 2;
	file_info fi;

	char current_dir_name[MAX_FILENAME_LEN + 1];
	u8 current_dir_name_len = 0;
	for (int i = 0; path[i]; i++) {
		if (path[i] == '/') {
			current_dir_name[current_dir_name_len] = 0;
			current_dir_name_len = 0;

			bool found_directory = FALSE;
			while (s_next_directory_file(&current_cluster, &current_entry_index, &fi)) {
				if (strcmp(current_dir_name, fi.filename) == 0) {
					found_directory = TRUE;
					break;
				}
			}

			if (!found_directory) {
				printf("Can't find \"%s\" folder", current_dir_name);
				return;
			}

			current_cluster = fi.first_cluster;
			current_entry_index = 0;
		} else {
			current_dir_name[current_dir_name_len++] = path[i];
		}
	}

	while (s_next_directory_file(&current_cluster, &current_entry_index, &fi)) {
		printf("%s| %s | Size: %d, Cluster: %d\n", fi.is_directory ? "DIR" : "FILE", fi.filename, fi.file_size, fi.first_cluster);
	}
}
