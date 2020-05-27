#include "VirtualMachine.h"
#include "Machine.h"
#include "Callback.h"
#include "Global.h"
#include "Skeleton.h"
#include "Manager.h"
#include "Mutex.h"
#include "FAT.h"
#include <stdio.h>
#include <math.h>
#include <string.h>  

extern "C" TVMMainEntry VMLoadModule(const char *module);
extern "C" void VMUnloadModule(void);

/*
VMSTART & TICK
*/

TVMStatus VMStart(int tickms, TVMMemorySize sharedsize, const char *mount, int argc, char *argv[]) {    
    TVMMainEntry main = VMLoadModule(argv[0]);
    if (!main) return VM_STATUS_FAILURE;    
    TVMStackBase base = (TVMStackBase)MachineInitialize(sharedsize);
    bool successfully_loaded_memory;
    global_init(tickms, sharedsize, base, successfully_loaded_memory);
    if (!successfully_loaded_memory) return VM_STATUS_FAILURE;    
    create_idle_thread();
    create_main_thread();    
    MachineRequestAlarm(1000*tickms, (TMachineAlarmCallback)&timer, NULL);                        
    MachineEnableSignals();    
    read_bpb(mount);
    main(argc, argv);
    VMUnloadModule();
    MachineTerminate();
    return VM_STATUS_SUCCESS;
}

TVMStatus VMTickMS(int *tickmsref) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (tickmsref == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    *tickmsref = tick_ms;
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;	        
};

TVMStatus VMTickCount(TVMTickRef tickref) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (tickref == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    *tickref = tick_count;
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;	 
};

/*
THREAD FUNCTIONS
*/

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (!entry || !tid) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    create_thread(entry, param, memsize, prio, tid);    
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadDelete(TVMThreadID thread) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    Thread* to_delete = all_threads[thread];
    if (to_delete == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;
    }else if (to_delete->state != VM_THREAD_STATE_DEAD) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    destroy_thread(thread); // check this?
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadActivate(TVMThreadID thread) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);    
    Thread* result = all_threads[thread];
    if (result == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;
    }else if (result->state != VM_THREAD_STATE_DEAD) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    SkeletonData* skeleton_data = new SkeletonData(result->entry, result->param);
    MachineContextCreate(result->context_ref(), skeleton_function, (void*)skeleton_data, (void*)result->memory, result->memsize);
    dispatch_and_schedule_if_needed(thread);
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadTerminate(TVMThreadID thread) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    Thread* result = all_threads[thread];
    if (result == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    if (result->state == VM_THREAD_STATE_DEAD) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    remove_thread_from_all_queues(thread);    
    result->state = VM_THREAD_STATE_DEAD;
    dead_queue.push_back(result);
    release_aquired_mutexes(thread);
    if (current_thread == thread) schedule();
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadID(TVMThreadIDRef threadref) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (threadref == NULL) {
        MachineResumeSignals(&state);    
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    *threadref = current_thread;
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (!stateref) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    Thread* result = all_threads[thread];
    if (result == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    *stateref = result->state;
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMThreadSleep(TVMTick tick) {
    TMachineSignalState state; 
    MachineSuspendSignals(&state);
    if (tick == VM_TIMEOUT_IMMEDIATE) {        
        dispatch_thread(current_thread);        
        schedule();
        MachineResumeSignals(&state);
        return VM_STATUS_SUCCESS;
    }    
    if (tick == VM_TIMEOUT_INFINITE) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;    
    }    
    all_threads[current_thread]->state = VM_THREAD_STATE_WAITING;    
    all_threads[current_thread]->timeout = tick;
    wait_queue.push_back(all_threads[current_thread]);        
    schedule();
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

/*
MUTEX FUNCTIONS
*/

TVMStatus VMMutexCreate(TVMMutexIDRef mutexref) {    
    TMachineSignalState state; 
    MachineSuspendSignals(&state);
    if (mutexref == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }            
    Mutex* mutex = new Mutex();       
    *mutexref = mutex->id;        
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexDelete(TVMMutexID mutex) {
    TMachineSignalState state; 
    MachineSuspendSignals(&state);
    Mutex* result = all_mutexes[mutex];
    if (result == nullptr) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;
    }   
    if (result->owner_id != VM_THREAD_ID_INVALID) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_STATE;
    } 
    delete result;
    all_mutexes[mutex] = nullptr;    
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref) {
    TMachineSignalState state; 
    MachineSuspendSignals(&state);
    if (ownerref == nullptr) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    if (mutex >= (int)all_mutexes.size()) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    Mutex* result = all_mutexes[mutex];       
    if (result->owner_id == VM_THREAD_ID_INVALID) {
        *ownerref = VM_THREAD_ID_INVALID;
        MachineResumeSignals(&state);
        return VM_STATUS_SUCCESS;
    }   
    *ownerref = result->owner_id;
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;    
}

TVMStatus VMMutexAcquire(TVMMutexID mutex, TVMTick timeout) {
    TMachineSignalState state; 
    MachineSuspendSignals(&state);    
    Mutex* result = all_mutexes[mutex];
    if (result == nullptr) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;    
    }
    if (timeout == VM_TIMEOUT_IMMEDIATE) {
        if (try_aquire_mutex_immediately(mutex)) {
            MachineResumeSignals(&state);
            return VM_STATUS_SUCCESS;
        }                
        MachineResumeSignals(&state);            
        return VM_STATUS_FAILURE;
    }    
    if (timeout == VM_TIMEOUT_INFINITE) {        
        aquire_mutex_no_matter_what(mutex);        
        MachineResumeSignals(&state);
        return VM_STATUS_SUCCESS;
    }
    aquire_mutex(mutex, timeout);    
    while (all_threads[current_thread]->state == VM_THREAD_STATE_WAITING) {}
    if (result->owner_id == current_thread) {
        MachineResumeSignals(&state);
        return VM_STATUS_SUCCESS;
    }
    MachineResumeSignals(&state);
    return VM_STATUS_FAILURE;
}

TVMStatus VMMutexRelease(TVMMutexID mutex) {
    TMachineSignalState state; 
    MachineSuspendSignals(&state);
    Mutex* result = all_mutexes[mutex];
    if (result == nullptr) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_ID;    
    }
    if (result->owner_id != all_threads[current_thread]->id) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_STATE;    
    }
    result->owner_id = VM_THREAD_ID_INVALID;
    get_next_mutex_owner(result->id);
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

/*
FILE FUNCTIONS
*/

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor) {
    return Internal_VMFileOpen(filename, flags, mode, filedescriptor);
}

TVMStatus VMFileClose(int filedescriptor) {
    return Internal_VMFileClose(filedescriptor);
}

TVMStatus VMFileRead(int filedescriptor, void *data, int *length) {
    return Internal_VMFileRead(filedescriptor, data, length);
}

TVMStatus VMFileWrite(int filedescriptor, void *data, int *length) {
    return Internal_VMFileWrite(filedescriptor, data, length);
}

TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset) {
    return Internal_VMFileSeek(filedescriptor, offset, whence, newoffset);
}

/*
INTERNAL VM FILE FUNCTIONS
*/

TVMStatus Internal_VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (filename == NULL || filedescriptor == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    Thread* thread = all_threads[current_thread];
    thread->state = VM_THREAD_STATE_WAITING;
    MachineFileOpen(filename, flags, mode, &file_callback, (void*)thread);  
    schedule();
    *filedescriptor = thread->file_io_result;     
    if(thread->file_io_result < 0) {
        MachineResumeSignals (&state);
        return VM_STATUS_FAILURE;
    }
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;    
}

TVMStatus Internal_VMFileClose(int filedescriptor) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);    
    Thread* thread = all_threads[current_thread];
    thread->state = VM_THREAD_STATE_WAITING;
    MachineFileClose(filedescriptor, file_callback, (void*)thread);
    schedule();
    if(thread->file_io_result < 0) {
        MachineResumeSignals (&state);
        return VM_STATUS_FAILURE;
    }
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus Internal_VMFileRead(int filedescriptor, void *data, int *length) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);
    if (data == NULL || length == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    MemoryBlock* block = memory_queue.front();                      
    VMMutexAcquire(block->mutex->id, VM_TIMEOUT_INFINITE);        
    Thread* thread = all_threads[block->mutex->owner_id];
    TVMStackBase write_pointer = (TVMStackBase)data;
    int num_chunks_to_read = (int)ceil(((float)*length / (float)VM_CHUNK_SIZE));
    int last_chunk_size = *length % VM_CHUNK_SIZE;
    if (last_chunk_size == 0) last_chunk_size = VM_CHUNK_SIZE;
    int total_bytes_read = 0;
    for (int i = 0; i < num_chunks_to_read; i++) {        
        int current_read_length = VM_CHUNK_SIZE;
        if (i == num_chunks_to_read-1) current_read_length = last_chunk_size;
        MachineFileRead(filedescriptor, block->stack_base, current_read_length, file_callback, (void*)thread);
        schedule();
        if(thread->file_io_result < 0) {
            MachineResumeSignals (&state);
            return VM_STATUS_FAILURE;
        }        
        memcpy(write_pointer, block->stack_base, current_read_length);
        write_pointer += current_read_length; 
        total_bytes_read += thread->file_io_result;       
    }
    *length = total_bytes_read;
    memory_queue.pop_front();
    memory_queue.push_back(block);        
    VMMutexRelease(block->mutex->id);
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus Internal_VMFileWrite(int filedescriptor, void *data, int *length) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);        
    if (data == NULL || length == NULL) {
        MachineResumeSignals(&state);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    MemoryBlock* block = memory_queue.front();                      
    VMMutexAcquire(block->mutex->id, VM_TIMEOUT_INFINITE);   
    Thread* thread = all_threads[block->mutex->owner_id];
    TVMStackBase read_pointer = (TVMStackBase)data;
    int num_chunks_to_write = (int)ceil(((float)*length / (float)VM_CHUNK_SIZE));
    int last_chunk_size = *length % VM_CHUNK_SIZE;
    if (last_chunk_size == 0) last_chunk_size = VM_CHUNK_SIZE;
    int total_bytes_written = 0;
    for (int i = 0; i < num_chunks_to_write; i++) {
        int current_write_length = VM_CHUNK_SIZE;
        if (i == num_chunks_to_write-1) current_write_length = last_chunk_size;
        memcpy(block->stack_base, read_pointer, current_write_length);
        MachineFileWrite(filedescriptor, block->stack_base, current_write_length, &file_callback, (void*)thread);
        if(thread->file_io_result < 0) {
            MachineResumeSignals (&state);
            return VM_STATUS_FAILURE;
        }        
        schedule();
        read_pointer += current_write_length;        
        total_bytes_written += thread->file_io_result;
    }
    *length = total_bytes_written;
    memory_queue.pop_front();
    memory_queue.push_back(block);        
    VMMutexRelease(block->mutex->id);
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}

TVMStatus Internal_VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset) {
    TMachineSignalState state;
    MachineSuspendSignals(&state);    
    Thread* thread = all_threads[current_thread];
    thread->state = VM_THREAD_STATE_WAITING;
    MachineFileSeek(filedescriptor, offset, whence, file_callback, (void*)thread);
    schedule();
    *newoffset = thread->file_io_result;
    if(thread->file_io_result < 0) {
        MachineResumeSignals (&state);
        return VM_STATUS_FAILURE;
    }
    MachineResumeSignals(&state);
    return VM_STATUS_SUCCESS;
}