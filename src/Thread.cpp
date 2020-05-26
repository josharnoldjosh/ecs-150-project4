#include "Thread.h"
#include "VirtualMachine.h"
#include "Machine.h"
#include "Global.h"

Thread::Thread(
    TVMThreadEntry &entry,
    void *param,
    TVMMemorySize memsize,
    TVMThreadPriority &prio,
    TVMThreadIDRef tid
    )
{    
    Thread::timeout = -1;
    Thread::state = VM_THREAD_STATE_DEAD;
    SMachineContext context;
    Thread::context = context;
    Thread::entry = entry;
    Thread::param = param;
    Thread::memsize = memsize;
    uint8_t *memory =  new uint8_t[memsize];
    Thread::memory = memory;
    Thread::priority = prio;    
    id = all_threads.size();
    *tid = id;    
    all_threads.push_back(this);
}

SMachineContextRef Thread::context_ref() {
    return &(Thread::context);
}