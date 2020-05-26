#ifndef __MUTEX__
#define __MUTEX__

#include "VirtualMachine.h"
#include "Thread.h"
#include <vector>
#include <deque>

using namespace std;

class Mutex {
    public:
        Mutex();
        TVMMutexID id;
        TVMThreadID owner_id;
        vector < deque < Thread* > > aquire_queue; 
};

#endif