#include "fat.h"
#include "directory.h"
#include "fat32_reserved_area.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROOT_DIR_CLUSTER 2
#define END_OF_CHAIN_CLUSTER 0x0FFFFFF8
#define CLUSTER_NUMBER_MASK 0x0FFFFFFF

static u32 s_cluster_size = 0; // cluster size in bytes
static u32 s_first_data_sector = 0; // number of first data sector
static FILE *s_fat_file = NULL;
static fat_boot_sector s_boot_sector;
static u32 *s_fat = NULL; // pointer to fat itself
static u32 s_current_directory_cluster = ROOT_DIR_CLUSTER;

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

static u32 s_get_cluster_from_path(char *path, u32 starting_cluster) {
	u32 current_cluster = starting_cluster;
	file_info fi;
	u32 cluster_count;

	char searched_directory[MAX_FILENAME_LEN + 1];
	char *path_ptr = path;

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
		bool directory_exists = directory_find_file(directory_clusters, s_cluster_size * cluster_count, &fi, searched_directory);
		if (!directory_exists || !fi.is_directory) {
			return 0;
		}

		current_cluster = fi.first_cluster;
		if (current_cluster == 0) {
			// ".." directory entry of the directories inside root directory point to cluster 0
			// but root directory starts at cluster 2
			current_cluster = ROOT_DIR_CLUSTER;
		}
	}

	return current_cluster;
}

bool fat_load_from_file(char *filepath) {
	s_fat_file = fopen(filepath, "rb+");
	if (!s_fat_file) {
		return FALSE;
	}

	fread(&s_boot_sector, sizeof(s_boot_sector), 1, s_fat_file);
	s_cluster_size = s_boot_sector.sector_size * s_boot_sector.sectors_per_cluster;
	s_first_data_sector = s_boot_sector.reserved_sectors + s_boot_sector.fats * s_boot_sector.fat32_length;

	s_fat = (u32*) malloc(s_boot_sector.sector_size * s_boot_sector.fat32_length);
	s_read_sectors(s_boot_sector.reserved_sectors, s_boot_sector.fat32_length, s_fat);

	return TRUE;
}

bool fat_change_current_directory(char *path) {
	u32 directory_cluster = 0;
	bool is_path_absolute = path[0] == '/';
	if (is_path_absolute) {
		directory_cluster = s_get_cluster_from_path(path + 1, ROOT_DIR_CLUSTER);
	} else {
		directory_cluster = s_get_cluster_from_path(path, s_current_directory_cluster);
	}

	if (!directory_cluster) {
		return FALSE;
	}

	s_current_directory_cluster = directory_cluster;

	return TRUE;
}

void fat_print_current_directory_files() {
	u32 current_entry_index = 0;
	file_info fi;
	u32 cluster_count;

	void *directory_clusters = s_prefetch_directory_clusters(s_current_directory_cluster, &cluster_count);
	while (directory_next_file(directory_clusters, s_cluster_size * cluster_count, &current_entry_index, &fi)) {
		printf("%s| %s | Size: %d, Cluster: %d\n", fi.is_directory ? "DIR" : "FILE", fi.filename, fi.file_size, fi.first_cluster);
	}
	printf("\n");
}

void fat_print_directory_files(char *absolute_path) {
	if (absolute_path[0] != '/') {
		printf("Incorrect path format\n");
	}

	u32 directory_cluster = s_get_cluster_from_path(absolute_path + 1, ROOT_DIR_CLUSTER);
	u32 current_entry_index = 0;
	file_info fi;
	u32 cluster_count;

	void *directory_clusters = s_prefetch_directory_clusters(directory_cluster, &cluster_count);
	while (directory_next_file(directory_clusters, s_cluster_size * cluster_count, &current_entry_index, &fi)) {
		printf("%s| %s | Size: %d, Cluster: %d\n", fi.is_directory ? "DIR" : "FILE", fi.filename, fi.file_size, fi.first_cluster);
	}
	printf("\n");
}

void fat_print_file_content(char *filename) {
	u32 cluster_count;
	file_info fi;

	void *directory_clusters = s_prefetch_directory_clusters(s_current_directory_cluster, &cluster_count);
	bool file_exists = directory_find_file(directory_clusters, s_cluster_size * cluster_count, &fi, filename);
	if (!file_exists || fi.is_directory) {
		printf("Can't find specified file\n");
		return;
	}

	u32 current_cluster = fi.first_cluster;
	u32 remaining_size = fi.file_size;
	do {
		u8 file_content_buffer[s_cluster_size + 1];
		s_read_clusters_into_buffer(current_cluster, 1, file_content_buffer);
		if (remaining_size >= s_cluster_size) {
			file_content_buffer[s_cluster_size] = 0;
			remaining_size -= s_cluster_size;
			printf(file_content_buffer);
		} else {
			file_content_buffer[remaining_size] = 0;
			printf(file_content_buffer);
			remaining_size = 0;
		}

		if (s_is_end_of_chain_cluster(current_cluster)) {
			break;
		}

		current_cluster = s_get_next_cluster_number(current_cluster);
	} while (remaining_size);

	printf("\n");
}
