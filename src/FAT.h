#ifndef __FAT__
#define __FAT__

#include "Machine.h"
#include "VirtualMachine.h"
#include <stdio.h>
#include <string>

using namespace std;

// should i replace some ints with strings?
#pragma pack(1)
typedef struct {
   uint8_t BS_jmpBoot[3];
   uint64_t BS_OEMName;
   uint16_t BPB_BytsPerSec;
   uint8_t BPB_SecPerClus;
   uint16_t BPB_RsvdSecCnt;
   uint8_t BPB_NumFATs;
   uint16_t BPB_RootEntCnt;
   uint16_t BPB_TotSec16;
   uint8_t BPB_Media;
   uint16_t BPB_FATSz16;
   uint16_t BPB_SecPerTrk;
   uint16_t BPB_NumHeads;
   uint32_t BPB_HiddSec;
   uint32_t BPB_TotSec32;
   uint8_t BS_DrvNum;
   uint8_t BS_Reserved1;  
   uint8_t BS_BootSig;
   uint32_t BS_VolID;
   uint8_t BS_VolLab[11]; 
   uint64_t BS_FilSysType;
} BPB;
#pragma pack()

typedef struct Directory {
	bool is_file;
   bool is_folder;
	string* name;
	SVMDirectoryEntry* entry_point;
	uint16_t firstClusterLo;
	vector <struct Directory* >* sub_directories;
} Directory;

void read_bpb(const char *mount);
void read_fat(const char *mount);
void create_root_directory(const char *mount);
SVMDirectoryEntry* create_entry(int offset);
Directory* create_directory(int offset, SVMDirectoryEntry* entry);
const char* create_short_file_name(int offset);
SVMDateTime* create_date_time(uint16_t time, uint16_t date);

#endif