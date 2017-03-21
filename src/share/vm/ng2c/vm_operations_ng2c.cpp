# include "ng2c/vm_operations_ng2c.hpp"

uint *
VM_NG2CMergeAllocCounters::_zeroed_counter_arr = NULL;

void
VM_NG2CMergeAllocCounters::doit()
{
  assert (!calling_thread()->is_VM_thread(), "should not be called by VMThread.");
  JavaThread * thread = (JavaThread*)calling_thread();

  // clear
  clear_counter_array();
  // save copy
  uint * thread_counters = thread->ngen_table();
  // cas-in the zeroed one
  // Atomic::cmpxchg_ptr((intptr_t)VM_NG2CMergeAllocCounters::_zeroed_counter_arr,
  //                     (intptr_t*)(((intptr_t)thread) + JavaThread::ngen_table_offset()),
  //                     (intptr_t)thread_counters);
  // do count
  
}

bool
VM_NG2CMergeAllocCounters::doit_prologue()
{
  return true;
}
