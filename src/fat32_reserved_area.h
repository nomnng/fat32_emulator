#ifndef FAT32_RESERVED_AREA_H
#define FAT32_RESERVED_AREA_H

#include "types.h"

typedef struct {
	u8 jmp[3]; // Boot strap short or near jump
	u8 system_id[8]; // OS that created the volume

	u16 sector_size; // bytes per logical sector
	u8 sectors_per_cluster; // sectors/cluster
	u16 reserved_sectors; // reserved area sector count
	u8 fats; // number of FATs, usually 2
	u16 root_dir_entries; // root directory entries, must be 0 on FAT32
	u16 sectors; // number of sectors, must be 0 on FAT32
	u8 media; // media code
	u16 fat_length; // size of FAT, must be 0 on FAT32
	u16 sectors_per_track; // sectors per track
	u16 heads; // number of heads
	u32 hidden; // hidden sectors (unused)
	u32 total_sectors; // total number of sectors of all areas

	/* The following fields are only used by FAT32 */
	u32 fat32_length; // size of FAT in sectors
	u16 flags; // bit 8: fat mirroring, low 4: active fat
	u8 version[2]; // major, minor filesystem version
	u32 root_cluster; // cluster number of the first cluster in root directory
	u16 info_sector; // sector number of filesystem info structure
	u16 backup_boot; // sector number of backup boot sector
	u8 reserved[12]; // Unused
	u8 drive_number; // drive number of the media (floppy or hard disk)
	u8 reserved1; // should be 0
	u8 boot_signature; // extended boot signature
	u32 volume_id;  // volume serial number
	u8 volume_label[11]; // voluma label
	u8 fat_name[8]; // FAT32 or FAT 16 or FAT 12
} __attribute__((packed)) fat_boot_sector;

typedef struct {
	u32 lead_signature; // 0x41615252 for validation that it's really fs_info
	u8 reserved[480];
	u32 structure_signature; // 0x61417272 another signature
	u32 free_cluster_count; // last known cluster count
	u32 next_free_cluster; // hint for where to look for free clusters
	u8 reserved2[12];
	u32 trail_signature; // 0xAA550000 ending signature 
} __attribute__((packed)) fs_info;

#endif