#include "Manager.h"
#include "Thread.h"
#include "Global.h"
#include "VirtualMachine.h"
#include "Machine.h"
#include "Skeleton.h"
#include "Callback.h"
#include <vector>
#include <stdio.h>
#include <algorithm>

/*
- Main thread is created with priority normal
- We set its state to running because its currently active
- We don't add to any queues (apart from all_threads which is done automatically)
- We just update the current_thread (which is an id type) to the main thread's id
- The ID's are overwritten
*/
void create_main_thread() {
    TVMThreadEntry entry = (TVMThreadEntry)&VMStart;
    void* param = (void*)0;
    TVMMemorySize memsize = 0;
    TVMThreadPriority prio = VM_THREAD_PRIORITY_NORMAL;
    TVMThreadID tid = 0;
    Thread* main_thread = new Thread(entry, param, memsize, prio, &tid);
    main_thread->state = VM_THREAD_STATE_RUNNING;    
    current_thread = main_thread->id;    
}

/*
- Idle thread is created with priority idle
- Has a large enough memory size
- We set state to ready and add it to the ready queue
- The ID's are overwritten
*/
void create_idle_thread() {
    TVMThreadEntry entry = (TVMThreadEntry)&idle;
    void* param = (void*)0;
    TVMMemorySize memsize = 0x100000;
    TVMThreadPriority prio = VM_THREAD_PRIORITY_IDLE;
    TVMThreadID tid = 0;
    Thread* thread = new Thread(entry, param, memsize, prio, &tid);
    thread->state = VM_THREAD_STATE_READY;    
    ready_queue[thread->priority].push_back(thread);
    MachineContextCreate(thread->context_ref(), thread->entry, thread->param, (void*)thread->memory, thread->memsize);
}

/*
- Threads are created in the dead state.
- We call MachineContextCreate to physically create the thread
*/
void create_thread(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid) {
    Thread* thread = new Thread(entry, param, memsize, prio, tid);
    dead_queue.push_back(thread);    
    return;
}

/*
- Clears a thread out of wait, dead and ready queues
- Input is thread_id
 - https://stackoverflow.com/questions/3385229/c-erase-vector-element-by-value-rather-than-by-position
*/
void remove_thread_from_all_queues(TVMThreadID thread) {    
    for (int i = 3; i >= 1; i--) {
        ready_queue[i].erase(std::remove(ready_queue[i].begin(), ready_queue[i].end(), all_threads[thread]), ready_queue[i].end());
    }
    wait_queue.erase(std::remove(wait_queue.begin(), wait_queue.end(), all_threads[thread]), wait_queue.end());
    dead_queue.erase(std::remove(dead_queue.begin(), dead_queue.end(), all_threads[thread]), dead_queue.end());
}

/*
- Completely destroys the thread as if it never existed
- First, removes from all queues (except all_threads)
- Then, sets the pointer to null in all_threads
- deletes the pointer
*/
void destroy_thread(TVMThreadID thread) {
    remove_thread_from_all_queues(thread);
    Thread* to_delete = all_threads[thread];
    all_threads[thread] = NULL;
    delete to_delete;
}

/*
- Removes a thread from all queues
- Sets its timeout to zero
- Sets its state to ready
- Pushes back into ready queue to the apropriate priority
*/
void dispatch_thread(TVMThreadID thread) {
    remove_thread_from_all_queues(thread);
    all_threads[thread]->state = VM_THREAD_STATE_READY;
    ready_queue[all_threads[thread]->priority].push_back(all_threads[thread]); 
};

void dispatch_and_schedule_if_needed(TVMThreadID thread) {
    dispatch_thread(thread);
    if (all_threads[thread]->priority > all_threads[current_thread]->priority) {
        dispatch_thread(current_thread);
        schedule();
    }
};

bool try_aquire_mutex_immediately(TVMMutexID mutex) {      
    Mutex* result = all_mutexes[mutex];    
    if (result->owner_id != VM_THREAD_ID_INVALID) return false;    
    // printf("Mutex id %d with thread id %d\n", mutex, current_thread);    
    result->owner_id = current_thread;    
    return true;
}

void aquire_mutex_no_matter_what(TVMMutexID mutex) {
    Mutex* result = all_mutexes[mutex];
    if (result->owner_id == VM_THREAD_ID_INVALID) {        
        try_aquire_mutex_immediately(mutex);             
    }else{        
        remove_thread_from_all_queues(current_thread);
        Thread* thread = all_threads[current_thread];        
        thread->state = VM_THREAD_STATE_WAITING;        
        result->aquire_queue[thread->priority].push_back(thread);        
        schedule();
        // We don't add the thread to the normal, global wait queue because its in waiting mode for infinity
    }    
}

void aquire_mutex(TVMMutexID mutex, TVMTick timeout) {
    Mutex* result = all_mutexes[mutex];
    if (result->owner_id == VM_THREAD_ID_INVALID) {        
        try_aquire_mutex_immediately(mutex);        
    }else{
        remove_thread_from_all_queues(current_thread);        
        Thread* thread = all_threads[current_thread];
        thread->state = VM_THREAD_STATE_WAITING;
        thread->timeout = timeout;
        wait_queue.push_back(thread);        
        result->aquire_queue[thread->priority].push_back(thread);                
        schedule();
    }    
}

void thread_timed_out_waiting_to_aquire_mutex(TVMThreadID thread) {
    for (int j = 0; j < (int)all_mutexes.size(); j++) {
        Mutex* mutex = all_mutexes[j];           
        for (int i = 0; i < (int)mutex->aquire_queue.size(); i++) {            
            for (int k = 0; k < (int)mutex->aquire_queue[i].size(); k++) {
                if (mutex->aquire_queue[i][k]->id == thread) {                    
                    mutex->aquire_queue[i].erase(mutex->aquire_queue[i].begin() + k);
                    k--;                    
                }    
            }            
        }
    }
}

void get_next_mutex_owner(TVMMutexID mutex) {
    // printf("———High: %d\n", (int)ready_queue[3].size());
    Mutex* result = all_mutexes[mutex];
    for (int i = 3; i >= 1; i--) {        
        if ((int)result->aquire_queue[i].size() == 0) continue;        
        Thread* next_owner = result->aquire_queue[i].front();
        if (next_owner == nullptr) continue;
        result->aquire_queue[i].pop_front();
        result->owner_id = next_owner->id;
        dispatch_thread(next_owner->id);                
        if (next_owner->priority > all_threads[current_thread]->priority) {            
            dispatch_thread(current_thread);
            schedule();
        }        
        break;
    }
}

void release_aquired_mutexes(TVMThreadID thread) {    
    for (int j = 0; j < all_mutexes.size(); j++) {
        Mutex* mutex = all_mutexes[j];
        if (mutex->owner_id == thread) {
            mutex->owner_id = VM_THREAD_ID_INVALID;
            get_next_mutex_owner(mutex->id);
        }
    }    
}