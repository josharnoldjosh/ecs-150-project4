#include "Callback.h"
#include "VirtualMachine.h"
#include "Machine.h"
#include "Global.h"
#include "Thread.h"
#include "Manager.h"
#include <stdio.h>
#include <algorithm>

void timer(void* data) {
    Thread* current = all_threads[current_thread];
    current->state = VM_THREAD_STATE_READY;
    ready_queue[current->priority].push_back(current);
    for (int i = 0;i < (int)wait_queue.size(); i++) {
        if (wait_queue[i]->timeout > 0) {               
            wait_queue[i]->timeout--;            
        }else{
            Thread* awake = wait_queue[i];
            awake->timeout = 0;
            awake->state = VM_THREAD_STATE_READY;
            ready_queue[awake->priority].push_back(awake);
            wait_queue.erase(wait_queue.begin() + i);
            thread_timed_out_waiting_to_aquire_mutex(awake->id);
            i--;
        }
    }    
    schedule();  
    tick_count++;    
}

void idle(void*) {    
    MachineEnableSignals();
    while(true);
}

void schedule() {

    // printf("\n\n Current thread %d\n", current_thread);
    // printf("\nDead: %d\n", (int)dead_queue.size());    
    // printf("High: %d\n", (int)ready_queue[3].size());
    // printf("Normal: %d\n", (int)ready_queue[2].size());
    // printf("Low: %d\n", (int)ready_queue[1].size());
    // printf("Idle: %d\n", (int)ready_queue[0].size());    
    // printf("Waiting: %d\n", (int)wait_queue.size());

    Thread* new_thread = NULL;
    for (int priority = 3; priority >= 0; priority--) {
        if (!ready_queue[priority].empty()) {    
            new_thread = ready_queue[priority].front();
            ready_queue[priority].pop_front();            
            break;
        }
    }    

    // printf("Switch from %d to %d\n", current_thread, new_thread->id);

    SMachineContextRef prev_context = all_threads[current_thread]->context_ref();
    SMachineContextRef new_context = new_thread->context_ref();     
    new_thread->state = VM_THREAD_STATE_RUNNING;
    current_thread = new_thread->id;    
    if (prev_context == new_context) return;
    MachineContextSwitch(prev_context, new_context);
}

void file_callback(void* call_data, int result) {    
    Thread* thread = (Thread*)call_data;
    thread->file_io_result = result;
    dispatch_and_schedule_if_needed(thread->id);
}
