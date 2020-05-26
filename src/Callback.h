#ifndef __CALLBACK__
#define __CALLBACK__

#include "VirtualMachine.h"

void timer(void* data);
void idle(void*);
void schedule();
void schedule_with_priority(TVMThreadPriority priority);
void file_callback(void* call_data, int result);

#endif