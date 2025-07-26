#include "directory.h"
#include "fat32_dir_entry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static void s_convert_utf16_to_ascii(char *ascii, u16 *utf16, int count) {
	for (int i = 0; i < count; i++) {
		if (utf16[i] == 0xFFFF) {
			break;
		}
		ascii[i] = utf16[i] > 127 ? '?' : utf16[i];
	}
}

static int s_read_lfn(long_dir_entry *long_entry, char *out_long_name) {
	if (!(long_entry->ord & LAST_LONG_ENTRY)) {
		out_long_name[0] = 0;
		return 1; // if first long entry is not LAST_LONG_ENTRY then ignore it 
	}

	u8 checksum = long_entry->checksum;
	int long_entries_count = long_entry->ord ^ LAST_LONG_ENTRY;
	for (u8 ord = long_entries_count; ord > 0; long_entry++, ord--) {
		if ((long_entry->ord & 0b111111) != ord || checksum != long_entry->checksum) {
			out_long_name[0] = 0; // NULL terminate string because lfn is damaged
			return (long_entries_count - ord + 1); // and return how many entries was already processed
		}

		char *current_part_ptr = out_long_name + 13 * (ord - 1);
		s_convert_utf16_to_ascii(current_part_ptr, long_entry->name1, 5);
		s_convert_utf16_to_ascii(current_part_ptr + 5, long_entry->name2, 6);
		s_convert_utf16_to_ascii(current_part_ptr + 11, long_entry->name3, 2);
	}

	out_long_name[13 * long_entries_count] = 0; // add NULL terminating character in case LFN doesn't have one
	return long_entries_count;
}

static void s_convert_sfn(char *in_name, char* out_name) {
	int out_name_length = 0;
	for (int i = 0; i < 8; i++) {
		if (in_name[i] == ' ') {
			break;
		} 
		out_name[out_name_length++] = in_name[i];
	}

	if (in_name[8] != ' ' || in_name[9] != ' ' || in_name[10] != ' ') {
		out_name[out_name_length++] = '.';
		for (int i = 8; i < 11; i++) {
			if (in_name[i] != ' ') {
				out_name[out_name_length++] = in_name[i];
			}
		}
	}
	out_name[out_name_length] = 0;
}

static u8 s_sfn_checksum(char *name) {
	u8 sum = 0;
	for (s16 i = 11; i != 0; i--) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *name++;
	}
	return sum;
}

static u8 s_calculate_lfn_entries_count(char *name) {
	if (!name[0]) {
		return 1;
	}
	return 1 + (strlen(name) - 1) / 13;
}

static void s_fill_long_dir_entry(long_dir_entry *long_entry, u8 ord, u8 checksum, char *name) {
	long_entry->ord = ord;
	long_entry->attributes = ATTR_LONG_NAME;
	long_entry->type = 0;
	long_entry->checksum = checksum;
	long_entry->first_cluster_low = 0;

	u32 name_offset = ((ord & 0b111111) - 1) * 13;
	name += name_offset;

	bool str_ended = FALSE;
	for (int i = 0; i < 13; i++) {
		u16 c;

		if (str_ended) {
			c = 0xFFFF;
		} else {
			if (name[i] == 0) {
				str_ended = TRUE;
			}
			c = name[i];
		}

		if (i < 5) {
			long_entry->name1[i] = c;
		} else if (i < 11) {
			long_entry->name2[i - 5] = c;
		} else {
			long_entry->name3[i - 11] = c;
		}
	}
}

int directory_count_files(void *ptr, int size) {
	int entry_count = 0;
	void *end_ptr = (char*) ptr + size;
	for (dir_entry *current_entry = ptr; current_entry < end_ptr; current_entry++) {
		u8 first_byte = current_entry->name[0];
		if (!first_byte) {
			break;
		} else if (first_byte == 0xE5) {
			continue;
		} else if ((current_entry->attributes & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) {
			continue;
		}

		entry_count++;
	}

	return entry_count;
}

bool directory_next_file(void *ptr, u32 size, u32 *out_current_entry_index, file_info *out_file_info) {
	dir_entry *current_entry = (dir_entry*) ptr + *out_current_entry_index;
	dir_entry *end_ptr = (dir_entry*) ((u8*) ptr + size);

	while (current_entry < end_ptr) {
		u8 first_byte = current_entry->name[0];
		if (!first_byte) {
			return FALSE;
		} else if (first_byte == 0xE5) {
			*out_current_entry_index += 1;
			current_entry++;
			continue;
		}

		if ((current_entry->attributes & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) {
			long_dir_entry *long_entry = (long_dir_entry*) current_entry;
			int long_entry_count = s_read_lfn(long_entry, out_file_info->filename);

			current_entry += long_entry_count;
			u8 sfn_checksum = s_sfn_checksum(current_entry->name);
			*out_current_entry_index += long_entry_count;
			if (!out_file_info->filename[0] || sfn_checksum != long_entry->checksum) {
				continue;
			}
		} else {
			s_convert_sfn(current_entry->name, out_file_info->filename);
		}
		
		out_file_info->file_size = current_entry->size;
		out_file_info->first_cluster = (current_entry->first_cluster_high << 16) | current_entry->first_cluster_low;
		out_file_info->is_directory = current_entry->attributes & ATTR_DIRECTORY;
		*out_current_entry_index += 1;

		return TRUE;
	}

	return FALSE;
}

bool directory_find_file(void *ptr, u32 size, file_info *out_file_info, char* name) {
	u32 current_entry_index = 0;
	while (directory_next_file(ptr, size, &current_entry_index, out_file_info)) {
		if (strcmp(name, out_file_info->filename) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}

u32 directory_calculate_dir_entry_size(char *directory_name) {
	u32 required_dir_entries = s_calculate_lfn_entries_count(directory_name) + 1; // +1 because of the sfn
	return required_dir_entries * sizeof(dir_entry);
}

void* directory_find_free_entry(void *ptr, u32 size) {
	dir_entry *end_ptr = (dir_entry*) ((u8*) ptr + size);
	for (dir_entry *current_entry = ptr; current_entry < end_ptr; current_entry++) {
		if (!current_entry->name[0]) {
			return current_entry;
		}
	}

	return NULL;
}

void directory_generate_sfn(void *ptr, u32 size, char* name, char* out_sfn) {
	memset(out_sfn, ' ', SFN_LEN);
	int name_len = strlen(name);

	char *name_ptr = name;
	for (int i = 0; i < 8; i++) {
		if (name_ptr[i] == '.' || name_ptr[i] == 0) {
			break;
		}

		if (name_ptr[i] == ' ') {
			name_ptr += 1;
			i--;
			continue;
		}

		out_sfn[i] = toupper(name_ptr[i]);
	}

	char *dot_ptr = strchr(name, '.');
	if (dot_ptr) {
		dot_ptr += 1;
		for (int i = 0; i < 3; i++) {
			if (!dot_ptr[i]) {
				break;
			}
			out_sfn[8 + i] = toupper(dot_ptr[i]);
		}
	}

	u32 trailing_i = 1;
	bool duplicate_exists;
	do {
		duplicate_exists = FALSE;
		dir_entry *end_ptr = (dir_entry*) ((u8*) ptr + size);
		for (dir_entry *current_entry = ptr; current_entry < end_ptr; current_entry++) {
			bool is_lfn = (current_entry->attributes & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME;
			if (current_entry->name[0] == 0xE5 || is_lfn) {
				continue;
			}

			if (strncmp(current_entry->name, out_sfn, SFN_LEN) == 0) {
				duplicate_exists = TRUE;

				char trailing_i_str[8] = "";
				sprintf(trailing_i_str, "~%d", trailing_i);

				u8 trailing_i_str_len = strlen(trailing_i_str);
				memcpy(out_sfn + (8 - trailing_i_str_len), trailing_i_str, trailing_i_str_len);

				trailing_i++;
				break;
			}
		}
	} while (duplicate_exists);
}


void directory_generate_dir_entry(void *buffer, char *directory_name, char *directory_sfn, u32 first_cluster) {
	u8 checksum = s_sfn_checksum(directory_sfn);
	u8 ord_counter = s_calculate_lfn_entries_count(directory_name);

	long_dir_entry *long_entry = buffer;
	s_fill_long_dir_entry(long_entry++, LAST_LONG_ENTRY | ord_counter--, checksum, directory_name);

	u32 remained_length = strlen(directory_name);
	while (ord_counter > 0) {
		s_fill_long_dir_entry(long_entry++, ord_counter--, checksum, directory_name);
	}

	dir_entry *short_entry = (dir_entry*) long_entry;
	memcpy(short_entry->name, directory_sfn, SFN_LEN);
	short_entry->attributes = ATTR_DIRECTORY;
	short_entry->nt_reserved = 0;
	short_entry->crt_time_tenth = 0;
	short_entry->crt_time = 0;
	short_entry->crt_date = 0;
	short_entry->last_access_date = 0;
	short_entry->write_time = 0;
	short_entry->write_date = 0;
	short_entry->first_cluster_high = first_cluster >> 16;
	short_entry->first_cluster_low = first_cluster & 0xFFFF;
	short_entry->size = 0;
}

void directory_generate_new_folder_dir_entries(void *buffer, u32 current_cluster, u32 parent_cluster) {
	memset(buffer, 0, sizeof(dir_entry) * 2);
	dir_entry *current_dir_entry = buffer;
	dir_entry *parent_dir_entry = current_dir_entry + 1;

	memcpy(current_dir_entry->name, ".          ", 11);
	current_dir_entry->attributes = ATTR_DIRECTORY;
	current_dir_entry->first_cluster_high = current_cluster >> 16;
	current_dir_entry->first_cluster_low = current_cluster & 0xFFFF;

	memcpy(parent_dir_entry->name, "..         ", 11);
	parent_dir_entry->attributes = ATTR_DIRECTORY;
	parent_dir_entry->first_cluster_high = parent_cluster >> 16;
	parent_dir_entry->first_cluster_low = parent_cluster & 0xFFFF;
}
