#ifndef FAT32_DIR_ENTRY_H
#define FAT32_DIR_ENTRY_H

#include "types.h"

typedef struct {
	u8 name[11]; // short name 
	u8 attributes; // attributes, 2 upper bits should be zero
	u8 nt_reserved; // reserved by windows nt, should be 0
	u8 crt_time_tenth; // tenth of a second when file was created
	u16 crt_time; // time when file was created
	u16 crt_date; // date when file was created
	u16 last_access_date; // last access date
	u16 first_cluster_high; // high word of entry's first cluster number
	u16 write_time; // time of last write
	u16 write_date; // date of last write
	u16 first_cluster_low; // low word of entry's first cluster number 
	u32 size; // file size
} __attribute__((packed)) dir_entry;

typedef struct {
	u8 ord; // sequence number of long entry, masked with 0x40 if it's the last one
	u16 name1[5]; // first 5 characters 
	u8 attributes; // attributes, 2 upper bits should be zero
	u8 type; // reserved for future uses, should be 0
	u8 checksum; // checksum for short name entry
	u16 name2[6]; // next 6 characters
	u16 first_cluster_low; // should be ZERO, needed for compatibility
	u16 name3[2]; // last 2 characters
} __attribute__((packed)) long_dir_entry;

#define ATTR_READ_ONLY 0x1 // read only file
#define ATTR_HIDDEN 0x2 // is hidden
#define ATTR_SYSTEM 0x4 // os file
#define ATTR_VOLUME_ID 0x8 // indicated this file is a label for the volume
#define ATTR_DIRECTORY 0x10 // is directory
#define ATTR_ARCHIVE 0x20 // might be used to determine which files was modified since last backup
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID) // long name entry
#define ATTR_LONG_NAME_MASK 0b00111111 // mask to check only 6 lower bits, because 2 upper bits needs to be ignored
#define LAST_LONG_ENTRY 0x40

#endif