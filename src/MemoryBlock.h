#ifndef __MEMORY_BLOCK__
#define __MEMORY_BLOCK__

#include "Mutex.h"
#include "VirtualMachine.h"

class MemoryBlock {
    public:
        MemoryBlock();
        Mutex* mutex;
        TVMStackBase stack_base;
};

#endif