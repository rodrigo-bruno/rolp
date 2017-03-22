#ifndef SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP
#define SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP

# include "runtime/vm_operations.hpp"
# include "ng2c/ng2c_globals.hpp"

// forward declaration
class G1CollectedHeap;

class VM_NG2CMergeAllocCounters : public VM_Operation
{
 private:
  static uint * _zeroed_counter_arr;

  // just a helper
  G1CollectedHeap * _g1h;

  // call from here, only.
  static void clear_counter_array() { memset((void*)_zeroed_counter_arr, 0, NG2C_MAX_ALLOC_SITE); }

 public:
  // There is no need to call the set_calling_thread since the VMThread::execute(&op)
  // will take care of it.
  VM_NG2CMergeAllocCounters(G1CollectedHeap * g1h) : _g1h(g1h) {  }
  
  virtual void doit();
  virtual bool doit_prologue();

  virtual VMOp_Type type() const       { return VMOp_NG2CMergeAllocCounters; }
  virtual Mode evaluation_mode() const { return _concurrent; }
  
};

#endif // SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP
