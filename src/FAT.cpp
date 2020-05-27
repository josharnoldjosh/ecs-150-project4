#include "FAT.h"
#include "VirtualMachine.h"
#include "Global.h"
#include <stdio.h>
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