#include "Global.h"
#include "Machine.h"
#include "Thread.h"
#include <deque>
#include <vector>
#include <stdio.h>
#include <math.h>       

using namespace std;

int tick_ms = 0;
volatile int tick_count = 0;
deque < Mutex* > all_mutexes;
deque < Thread* > all_threads;
vector < deque < Thread* > > ready_queue;
deque < Thread* > wait_queue;
deque < Thread* > dead_queue;
deque < MemoryBlock* > memory_queue;
TVMThreadID current_thread;
BPB* bpb;

void init_tick_ms(int tickms) {
    tick_ms = tickms;
}

void init_ready_queue() {
    for (int i = 0; i < 4; i++) {         
        ready_queue.push_back(deque< Thread* >());        
    }
}

void init_shared_memory(TVMMemorySize sharedsize, TVMStackBase &base, bool &success) {
    int num_chunks_to_init = (int)ceil((float)sharedsize / (float)VM_CHUNK_SIZE);    
    for (int i = 0; i < num_chunks_to_init; i++) {
        MemoryBlock* block = new MemoryBlock();
        block->mutex = new Mutex();
        block->stack_base = base;
        base += VM_CHUNK_SIZE;
        memory_queue.push_back(block);
        memory_queue.push_back(block);
    }
    success = true;
}

void init_bpb() {
    bpb = new BPB();
}

void global_init(int tickms, TVMMemorySize sharedsize, TVMStackBase &base, bool &success) {
    init_tick_ms(tickms);
    init_ready_queue();
    init_shared_memory(sharedsize, base, success);        
    init_bpb();
}