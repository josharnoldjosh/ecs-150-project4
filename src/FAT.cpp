#include "FAT.h"
#include "VirtualMachine.h"
#include "Global.h"
#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>

void read_bpb(const char *mount) {
    int fd = 0;    
    TVMStatus open_status = Internal_VMFileOpen(mount, O_RDWR, S_IWOTH, &fd);    
    int size = (int)sizeof(bpb);
    TVMStatus read_status = Internal_VMFileRead(fd, (void*)bpb, &size);
    TVMStatus close_status = Internal_VMFileClose(fd);
    if (open_status == VM_STATUS_FAILURE || read_status == VM_STATUS_FAILURE || close_status == VM_STATUS_FAILURE) {
        printf("Opening, reading, or closing failed!\n");
    }    
    return;
}

void read_fat(const char *mount) {
    int start_pointer = bpb->BPB_BytsPerSec * bpb->BPB_RsvdSecCnt;
    fat = new uint8_t[bpb->BPB_FATSz16];       
    int fd = 0;
    TVMStatus open_status = Internal_VMFileOpen(mount, O_RDWR, S_IWOTH, &fd);
    int size = (int)sizeof(bpb->BPB_FATSz16);
    TVMStatus read_status = Internal_VMFileRead(fd, (void*)fat, &size);
    TVMStatus close_status = Internal_VMFileClose(fd);
    if (open_status == VM_STATUS_FAILURE || read_status == VM_STATUS_FAILURE || close_status == VM_STATUS_FAILURE) {
        printf("Opening, reading, or closing failed!\n");
    }    
    return;
}

void create_root_directory(const char *mount) {
    int first_sector_offset = bpb->BPB_BytsPerSec * (bpb->BPB_RsvdSecCnt + bpb->BPB_NumFATs * bpb->BPB_FATSz16);
    root_directory_pointer = new uint8_t[bpb->BPB_RootEntCnt * 32];
    int fd = 0;
    TVMStatus open_status = Internal_VMFileOpen(mount, O_RDWR, S_IWOTH, &fd);
    int size = (int)sizeof(root_directory_pointer);
    TVMStatus read_status = Internal_VMFileRead(fd, (void*)root_directory_pointer, &size);
    TVMStatus close_status = Internal_VMFileClose(fd);
    if (open_status == VM_STATUS_FAILURE || read_status == VM_STATUS_FAILURE || close_status == VM_STATUS_FAILURE) {
        printf("Opening, reading, or closing failed!\n");
    }   

    Directory* root = new Directory();
    root->is_file = false;
    root->is_folder = true;
    root->sub_directories = new std::vector<Directory*>();
    root->entry_point = NULL;
    root->name = new std::string("/");
    directories->push_back(root);

    for(int i = 0; i < bpb->BPB_RootEntCnt; i += 32){
		if (root_directory_pointer[i + 11] == 15) continue;
        SVMDirectoryEntry* entry = create_entry(i);


        // root->childEntries->push_back(directoryEntry);			
	}
}

SVMDirectoryEntry* create_entry(int offset) {
    SVMDirectoryEntry* entry = new SVMDirectoryEntry();
    const char* short_file_name = create_short_file_name(offset);
    strcpy(entry->DShortFileName, short_file_name);
    entry->DAttributes = root_directory_pointer[offset + 11];
    uint16_t time = (root_directory_pointer[offset + 15] << 8) | root_directory_pointer[offset + 14];		
	uint16_t date = (root_directory_pointer[offset + 17] << 8) | root_directory_pointer[offset + 16];
	SVMDateTime* create = create_date_time(time, date);					
	create->DHundredth = (unsigned char)root_directory_pointer[offset + 13];
	time = 0;
	date = (root_directory_pointer[offset + 19] << 8) | root_directory_pointer[offset + 18];				
	SVMDateTime* access = create_date_time(time, date);
	access->DHundredth = 0;
	time = (root_directory_pointer[offset + 23] << 8) | root_directory_pointer[offset + 22];				
	date = (root_directory_pointer[offset + 25] << 8) | root_directory_pointer[offset + 24];
	SVMDateTime* write = create_date_time(time, date);
	write->DHundredth = 0;
	entry->DCreate = *create;
	entry->DAccess = *access;
	entry->DModify = *write;
	entry->DSize = (unsigned int)((root_directory_pointer[offset + 31] << 24) | (root_directory_pointer[offset + 30] << 16) | (root_directory_pointer[offset + 29] << 8) | root_directory_pointer[offset + 28]);
	return entry;
}

Directory* create_directory(int offset, SVMDirectoryEntry* entry) {
    
}

const char* create_short_file_name(int offset) {
    char name[13];															
	for(int j = 0; j < 8; j++) name[j] = root_directory_pointer[offset + j];	
	for(int j = 8; j < 11; j++) name[j + 1] = root_directory_pointer[offset + j];
	name[12] = '\0';
	name[8] = '.';    
	std::string short_file_name = std::string((const char*)name);
	for(int i = 0; i < (int)short_file_name.size(); i++){
		if(((int)short_file_name[i]) == 32){
			short_file_name.erase(i, 1);
			i--;
		}
	}	
	if(short_file_name[short_file_name.length() - 1] == '.'){		
		short_file_name.erase(short_file_name.length() - 1, 1);
	}		
	return short_file_name.c_str();
}

SVMDateTime* create_date_time(uint16_t time, uint16_t date) {	
	SVMDateTime* date_time = new SVMDateTime();	
	date_time->DYear = (unsigned int)(((date & 0xFE00) >> 9) + 1980);
	date_time->DMonth = (unsigned char)((date & 0x01E0) >> 5);	
	date_time->DDay = (unsigned char)(date & 0x001F);			
	date_time->DHour = (unsigned char)((time & 0xF800) >> 11);	
	date_time->DMinute = (unsigned char)((time & 0x05E0) >> 5);		
	date_time->DSecond = (unsigned char)(time & 0x001F);				
	return date_time;
}