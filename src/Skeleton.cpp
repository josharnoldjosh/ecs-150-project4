#include "Skeleton.h"
#include "VirtualMachine.h"
#include "Global.h"
#include "Machine.h"
#include <stdio.h>

SkeletonData::SkeletonData(TVMThreadEntry &entry, void* param) {
    SkeletonData::entry = entry;
    SkeletonData::param = param;
};

void skeleton_function(void* data) {
    MachineEnableSignals();
    SkeletonData* skeleton = (SkeletonData*)data;
    TVMThreadEntry entry = skeleton->entry;
    void* param = skeleton->param;
    entry(param);
    VMThreadTerminate(current_thread);    
};