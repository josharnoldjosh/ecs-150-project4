#ifndef __FAT__
#define __FAT__

#include "Machine.h"

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

void read_bpb(const char *mount);

#endif