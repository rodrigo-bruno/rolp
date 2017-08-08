#ifndef SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP
#define SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP

# include "runtime/vm_operations.hpp"
# include "ng2c/ng2c_globals.hpp"

class NG2C_MergeAllocCounters;
class NG2C_MergeWorkerThreads;

class NG2C_MergeWorkerThreads : public ThreadClosure
{
 private:
  NG2C_MergeAllocCounters * _op;

 public:
  NG2C_MergeWorkerThreads(NG2C_MergeAllocCounters * op) : _op(op) { }

  void do_thread(Thread * thread);
};

class NG2C_MergeAllocCounters : public VM_Operation
{
  // Note: necessary so that these closures can call private methods.
  friend class NG2C_MergeWorkerThreads;

 private:
  // Note: number of times this vm operation ran.
  static uint   _total_update_target_gen;

  // TODO - this methods should be part of closures not the operation...
  void update_promotions(PromotionCounter * global, PromotionCounter * survivors);
  void update_promotions(NamedThread * thread);
  bool should_inc_gen(PromotionCounter * pc);
  bool should_use_context(PromotionCounter * pc);
  void update_target_gen();
  unsigned int get_cur_tenuring_threshold();

 public:
  // There is no need to call the set_calling_thread since the VMThread::execute(&op)
  // will take care of it.
  NG2C_MergeAllocCounters() { }

  virtual void doit()
    {
      assert (!calling_thread()->is_VM_thread(), "should not be called by VMThread.");
      
      // Only update target gen every NG2C_GEN_ARRAY_SIZE gc cycles.
      _total_update_target_gen++;
      if (_total_update_target_gen % NG2CUpdateThreshold == 0) {
        // Sum up promotion counters.
        {
          NG2C_MergeWorkerThreads mwt_cljr(this);
          MutexLocker mu(Threads_lock);
          Threads::threads_do(&mwt_cljr);
        }

#ifdef DEBUG_NG2C_PROF_VMOP
        {
          gclog_or_tty->print_cr("[ng2c-vmop] cur_tenuring_threshold=%u",
             get_cur_tenuring_threshold());

          gclog_or_tty->print_cr("[ng2c-vmop] <printing promo counters>");
          Universe::promotion_counters()->print_on(gclog_or_tty);
          gclog_or_tty->print_cr("[ng2c-vmop] <printing promo counters> done!");

          gclog_or_tty->print_cr("[ng2c-vmop] <printing alloc counters>");
          Universe::method_bci_hashtable()->print_on(gclog_or_tty);
          gclog_or_tty->print_cr("[ng2c-vmop] <printing alloc counters> done!");

        }
#endif

        // Try to expand contexts or increment gens.
        update_target_gen();
      }
    }

  virtual bool doit_prologue();

  virtual VMOp_Type type() const          { return VMOp_NG2CMergeAllocCounters; }
  virtual Mode evaluation_mode() const    { return _concurrent; }
  virtual bool is_cheap_allocated() const { return true; }

};

#endif // SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP
