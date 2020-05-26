#include "Mutex.h"
#include "VirtualMachine.h"
#include "Global.h"
#include <stdio.h>
#include <vector>
#include <deque>

Mutex::Mutex() {    
    owner_id = VM_THREAD_ID_INVALID;       
    Mutex::id = all_mutexes.size();    
    all_mutexes.push_back(this);
    for (int i = 0; i < 4; i++) {         
        Mutex::aquire_queue.push_back(deque< Thread* >());
    }
}