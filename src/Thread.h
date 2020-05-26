#ifndef __THREAD__
#define __THREAD__

#include "VirtualMachine.h"
#include "Machine.h"

class Thread {
    public:
        Thread(
            TVMThreadEntry &entry,
            void *param,
            TVMMemorySize memsize,
            TVMThreadPriority &prio,
            TVMThreadIDRef tid
            );
        SMachineContext context;
        SMachineContextRef context_ref();
        TVMThreadID id;        
        TVMThreadPriority priority;
        TVMThreadState state;
        int timeout;
        uint8_t *memory;
        TVMMemorySize memsize;
        TVMThreadEntry entry;
        void *param;
        int file_io_result;
};

#endif