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

static u32 s_normalize_cluster_number(u32 raw_cluster_number) {
	return (raw_cluster_number & CLUSTER_NUMBER_MASK) - 2;
}

static void s_read_sectors(u32 offset, u32 count, void *dst_buffer) {
	fseek(s_fat_file, offset * s_boot_sector.sector_size, SEEK_SET);
	fread(dst_buffer, s_boot_sector.sector_size, count, s_fat_file);
}

static void s_read_clusters_into_buffer(u32 raw_cluster_number, u32 count, void *dst_buffer) {
	u32 cluster_number = s_normalize_cluster_number(raw_cluster_number);
	fseek(s_fat_file, s_first_data_sector * s_boot_sector.sector_size + cluster_number * s_cluster_size, SEEK_SET);
	fread(dst_buffer, s_cluster_size, count, s_fat_file);
}

static bool s_is_end_of_chain_cluster(u32 raw_cluster_number) {
	return (raw_cluster_number & CLUSTER_NUMBER_MASK) >= END_OF_CHAIN_CLUSTER;
}

static u32 s_get_next_cluster_number(u32 raw_cluster_number) {
	return s_fat[raw_cluster_number & CLUSTER_NUMBER_MASK];
}

static void* s_prefetch_directory_clusters(u32 starting_cluster, u32 *out_cluster_count) {
	u32 next_cluster = starting_cluster;
	u32 cluster_count = 1;
	while (next_cluster = s_get_next_cluster_number(next_cluster)) {
		if (s_is_end_of_chain_cluster(next_cluster)) {
			break;
		}
		cluster_count++;
	}

	void *directory_clusters_buffer = malloc(cluster_count * s_cluster_size);
	u32 current_cluster = starting_cluster;
	for (int i = 0; i < cluster_count; i++, current_cluster = s_get_next_cluster_number(current_cluster)) {
		u8 *buffer_ptr = ((u8*) directory_clusters_buffer) + s_cluster_size * i;
		s_read_clusters_into_buffer(current_cluster, 1, buffer_ptr);
	}

	*out_cluster_count = cluster_count;
	return directory_clusters_buffer;
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
	u32 cluster_count;

	if (path[0] != '/') {
		printf("Incorrect path format\n");
	}

	char searched_directory[MAX_FILENAME_LEN + 1];
	char *path_ptr = path + 1;

	while (path_ptr[0]) {
		char *slash_ptr = strchr(path_ptr, '/');
		if (slash_ptr) {
			int searched_directory_len = slash_ptr - path_ptr;
			memcpy(searched_directory, path_ptr, searched_directory_len);
			searched_directory[searched_directory_len] = 0;

			path_ptr = slash_ptr + 1;
		} else {
			int remaining_len = strlen(path_ptr);			
			memcpy(searched_directory, path_ptr, remaining_len);
			searched_directory[remaining_len] = 0;

			path_ptr += remaining_len;
		}

		void *directory_clusters = s_prefetch_directory_clusters(current_cluster, &cluster_count);
		if (!find_file_in_directory(directory_clusters, s_cluster_size * cluster_count, &fi, searched_directory)) {
			printf("Can't find \"%s\" folder\n", searched_directory);
			return;
		}

		current_cluster = fi.first_cluster;
		if (current_cluster == 0) {
			// ".." directory entry of the directories inside root directory point to cluster 0
			// but root directory starts at cluster 2
			current_cluster = 2;
		}
	}

	void *directory_clusters = s_prefetch_directory_clusters(current_cluster, &cluster_count);
	while (next_cluster_file(directory_clusters, s_cluster_size * cluster_count, &current_entry_index, &fi)) {
		printf("%s| %s | Size: %d, Cluster: %d\n", fi.is_directory ? "DIR" : "FILE", fi.filename, fi.file_size, fi.first_cluster);
	}
	printf("\n");
}
