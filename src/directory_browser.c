#include "directory_browser.h"
#include "fat32_dir_entry.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

int count_directory_files(void *ptr, int size) {
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

bool next_cluster_file(void *ptr, u32 size, u32 *out_current_entry_index, file_info *out_file_info) {
	dir_entry *current_entry = (dir_entry*) ptr + *out_current_entry_index;
	dir_entry *end_ptr = (dir_entry*) ((char*) ptr + size);

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
