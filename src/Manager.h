/*
The goal of this class is to handle creating, destorying, and working with moving threads around.
This is so the VirtualMachine.cpp only has to call one function instead of having lots of functions going on.
*/

#ifndef __MANAGER__
#define __MANAGER__

#include "VirtualMachine.h"
#include "Thread.h"

void create_main_thread();
void create_idle_thread();
void create_thread(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
void remove_thread_from_all_queues(TVMThreadID thread);
void destroy_thread(TVMThreadID thread);
void dispatch_thread(TVMThreadID thread);
void dispatch_and_schedule_if_needed(TVMThreadID thread);
bool try_aquire_mutex_immediately(TVMMutexID mutex);
void aquire_mutex_no_matter_what(TVMMutexID mutex);
void aquire_mutex(TVMMutexID mutex, TVMTick timeout);
void thread_timed_out_waiting_to_aquire_mutex(TVMThreadID thread);
void get_next_mutex_owner(TVMMutexID mutex);
void release_aquired_mutexes(TVMThreadID thread);

#endif