#ifndef SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP
#define SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP

# include "runtime/vm_operations.hpp"
# include "ng2c/ng2c_globals.hpp"

class NG2C_MergeAllocCounters;
class NG2C_SetThreadsContext;
class NG2C_MergeWorkerThreads;

class NG2C_MergeWorkerThreads : public ThreadClosure
{
 private:
  NG2C_MergeAllocCounters * _op;

 public:
  NG2C_MergeWorkerThreads(NG2C_MergeAllocCounters * op) : _op(op) { }

  void do_thread(Thread * thread);
};

class NG2C_SetThreadsContext : public ThreadClosure
{
 public:
  void do_thread(Thread * thread);
};

class NG2C_MergeAllocCounters : public VM_Operation
{
  // Note: necessary so that these closures can call private methods.
  friend class NG2C_MergeWorkerThreads;

 private:
  // Note: number of times this vm operation ran.
  static uint   _total_update_target_gen;
  bool          _need_more_context;

  // TODO - this methods should be part of closures not the operation...
  void update_promotions(PromotionCounter * global, PromotionCounter * survivors);
  void update_promotions(NamedThread * thread);
  void update_target_gen();
  unsigned int get_cur_tenuring_threshold();
  int normalize_derive_analyze(PromotionCounter * pc);

 public:
  // There is no need to call the set_calling_thread since the VMThread::execute(&op)
  // will take care of it.
  NG2C_MergeAllocCounters() : _need_more_context(false) { }

  virtual void doit()
    {
      assert (!calling_thread()->is_VM_thread(), "should not be called by VMThread.");

      // Avoid running vmop if jvm is not using NG2C profiler.
      if (NG2CStaticAnalysis == NULL) { return; }

      // Only update target gen every NG2C_GEN_ARRAY_SIZE gc cycles, and avoid
      // the first one because the JVM is still warming up.
      _total_update_target_gen++;

      gclog_or_tty->print_cr("[ng2c-vmop] gc=%u cur_tenuring_threshold=%u",
               _total_update_target_gen, get_cur_tenuring_threshold());

      if (_total_update_target_gen % NG2CUpdateThreshold == 0 && should_profile()) {
        // Sum up promotion counters.
        {
          NG2C_MergeWorkerThreads mwt_cljr(this);
          // Note: this lock is necessary if this vm_op is concurrent.
          // MutexLocker mu(Threads_lock);
          Threads::threads_do(&mwt_cljr);
        }

        if (_total_update_target_gen / NG2CUpdateThreshold > 1) {

          // Try to expand contexts or increment gens.
          update_target_gen();

          if (_need_more_context) {
            //Universe::static_analysis()->more_context(_need_more_context);
            gclog_or_tty->print_cr("[ng2c-vmop] need more context!");
          } else {
            //Universe::static_analysis()->more_context(_need_more_context);
            gclog_or_tty->print_cr("[ng2c-vmop] do not need more context!");
          }

#ifdef DEBUG_NG2C_PROF_VMOP
          {
            gclog_or_tty->print_cr("[ng2c-vmop] cur_tenuring_threshold=%u",
               get_cur_tenuring_threshold());
            gclog_or_tty->print_cr("[ng2c-vmop] <printing vmop counters>");
            Universe::promotion_counters()->print_on(gclog_or_tty);
            gclog_or_tty->print_cr("[ng2c-vmop] <printing vmop counters> done!");
          }
#endif
        }

        // Note: we clean the arry to ensure that we look at a single time window.
        // Note 2: However, it cramps up the evolution since after a cleanup we
        // will see objects that were allocated previously and are now with age
        // 2 (for example). This messes up the algorithm that determines the
        // inc gen or context. We should have a counter and only account for
        // survivors of the current "epoch".
        // Note 3: The NG2CUpdateThreshold should be set to at least 16 to avoid
        // the aforementioned problem. No object will be in the pipeline whose
        // allocation was not tracked.
        Universe::method_bci_hashtable()->zero();
        Universe::promotion_counters()->zero();

        // TODO - do we need to do this everytime?
        // TODO - we might need to do it everytime we change something?
        {
          NG2C_SetThreadsContext stc_cljr;
          // Note: this lock is necessary if this vm_op is concurrent.
          // MutexLocker mu(Threads_lock);
          Threads::threads_do(&stc_cljr);
        }
      }

      if (!should_profile()) {
        MaxTenuringThreshold = 1;
      }
    }

  virtual bool doit_prologue();
  virtual VMOp_Type type() const          { return VMOp_NG2CMergeAllocCounters; }
  //virtual Mode evaluation_mode() const    { return _concurrent; }
  virtual Mode evaluation_mode() const    { return _safepoint; }
  virtual bool is_cheap_allocated() const { return true; }

  // Note: profiling can be done only at start to understand the workload dynamics.
  // Note 2: profiling can be turned back on if pausetimes increase.
  static bool should_profile()            { return _total_update_target_gen / NG2CUpdateThreshold <= 3; }
  //static bool should_profile()            { return true; }
};

#endif // SHARE_VM_NG2C_VM_OPERATIONS_NG2C_HPP
