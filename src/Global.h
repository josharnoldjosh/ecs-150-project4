#ifndef __GLOBAL__
#define __GLOBAL__

#include "Machine.h"
#include "Thread.h"
#include "Mutex.h"
#include "MemoryBlock.h"
#include "FAT.h"
#include <vector>
#include <deque>

using namespace std;

extern int tick_ms;
extern volatile int tick_count;
extern deque < Mutex* > all_mutexes;
extern deque < Thread* > all_threads;
extern vector < deque < Thread* > > ready_queue; 
extern deque < Thread* > wait_queue;
extern deque < Thread* > dead_queue;
extern deque < MemoryBlock* > memory_queue;
extern TVMThreadID current_thread;
void global_init(int tickms, TVMMemorySize sharedsize, TVMStackBase &base, bool &success);
extern BPB* bpb;
extern uint8_t *fat;
extern uint8_t* root_directory_pointer;
extern vector< Directory* >* directories;

#endif