#ifndef __SKELETON__
#define __SKELETON__

#include "VirtualMachine.h"

class SkeletonData {
    public:
        SkeletonData(TVMThreadEntry &entry, void* param);
        TVMThreadEntry entry;
        void* param;        
};

void skeleton_function(void* data);

#endif