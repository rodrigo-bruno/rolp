# include "ng2c/vm_operations_ng2c.hpp"

uint * NG2C_MergeAllocCounters::_swp_counter_arr = NULL;
uint * NG2C_MergeAllocCounters::_inc_counter_arr = NULL;

void
NG2C_MergeJavaThreads::do_thread(Thread * thread)
{
  if (!thread->is_Java_thread()) return;
  _op->increment_allocations((JavaThread*)thread);
}

void
NG2C_MergeWorkerThreads::do_thread(Thread * thread)
{
  if (!thread->is_Worker_thread()) return;
  _op->update_promotions((WorkerThread*)thread);
}


void
NG2C_MergeAllocCounters::update_promotions(NGenerationArray * global, NGenerationArray * survivors)
{
  assert (global != NULL, "global array should not be NULL");
  assert (survivors != NULL, "survivors array should not be NULL");
  ngen_t * garr = global->array();
  ngen_t * sarr = survivors->array();
  ngen_t * end_sarr = sarr + survivors->size();

  // Note: survivors array will never have a value for the first position. This
  // is leveraged to speed up the next for cicle.
  sarr++;
  for (int i = 0; i < NG2C_GEN_ARRAY_SIZE; i++) {
    if (sarr < end_sarr && *sarr) {
      *garr -= *sarr;
      *++garr += *sarr++;
    }
  }
  memset((void*)survivors->array(), 0, sizeof(ngen_t) * NG2C_GEN_ARRAY_SIZE);
}

void
NG2C_MergeAllocCounters::update_promotions(WorkerThread * thread)
{
  MethodBciHashtable * hashtable = thread->method_bci_hashtable();
  MethodBciHashtable * global_hashtable = Universe::method_bci_hashtable();

  for (int i = 0; i < hashtable->table_size(); i++) {
    MethodBciEntry * surv = (MethodBciEntry*)hashtable->bucket(i);
    for (; surv != NULL; surv = surv->next()) {
      NGenerationArray * surv_arr = surv->literal();
      uint hash = surv_arr->hash();
      NGenerationArray * glbl_arr = global_hashtable->get_entry(hash);

#ifdef DEBUG_NG2C_PROF_VMOP
      for (int i = 0; i < NG2C_GEN_ARRAY_SIZE; i++)
        if (surv_arr->array()[i])
          gclog_or_tty->print_cr("[ng2c-vmop] <promotions> %s hash=%u age=%d promotions=%lu",
             glbl_arr == NULL ? "unkown" : "", hash, i, surv_arr->array()[i]);
#endif
      // Note: some hashes might get corrupted. If this happens, survivors will
      // register a hash that is not valid, leading to a null global array.
      if (glbl_arr != NULL) update_promotions(glbl_arr, surv_arr);
    }
  }
}

void
NG2C_MergeAllocCounters::increment_allocations(JavaThread* thread)
{
  uint * cswp = (uint*) thread->ngen_table();
  uint * cinc = _inc_counter_arr;

  // Note: atomically exchanging our swap buffer (zeroed) with the one
  // the thread is using.
  Atomic::cmpxchg_ptr(
     (void*)_swp_counter_arr,
     (volatile void**) thread->ngen_table_addr(),
     (void*)thread->ngen_table());

  assert(cswp != thread->ngen_table(), "atomic cmp xchng failed for thread's ngen table");
  _swp_counter_arr = cswp;

  for (int i = 0; i < NG2C_MAX_ALLOC_SITE; i++) *cinc++ += *cswp++;
  memset((void*)_swp_counter_arr, 0, sizeof(uint) * NG2C_MAX_ALLOC_SITE);
}

void
NG2C_MergeAllocCounters::update_allocations()
{
  MethodBciHashtable * global_hashtable = Universe::method_bci_hashtable();
  uint * gen_mapping = Universe::thread_gen_mapping()->hashes();
  uint * cinc = _inc_counter_arr;

#ifdef DEBUG_NG2C_PROF_VMOP
  for (int i = 0; i < NG2C_MAX_ALLOC_SITE; i++, cinc++) {
    if (*cinc) {
      gclog_or_tty->print_cr("[ng2c-vmop] <new allocations> hash_position=%u allocations=%u",
         i, *cinc);
    }
  }
  for (int i = 0; i < NG2C_MAX_ALLOC_SITE; i++, gen_mapping++) {
    if (*gen_mapping) {
      gclog_or_tty->print_cr("[ng2c-vmop] <position mappings> hash_position=%u hash=%u",
         i, *gen_mapping);
    }
  }
  gen_mapping = Universe::thread_gen_mapping()->hashes();
  cinc = _inc_counter_arr;
#endif

  for (int i = 0; i < NG2C_MAX_ALLOC_SITE; i++, cinc++, gen_mapping++) {
    if (*cinc) {
      assert (*gen_mapping != 0, "gen mapping should not be zero");
      global_hashtable->get_entry(*gen_mapping)->array()[0] += *cinc;
    }
  }
}

void
NG2C_MergeAllocCounters::update_target_gen()
{
  MethodBciHashtable * hashtable = Universe::method_bci_hashtable();

  for (int i = 0; i < hashtable->table_size(); i++) {
    MethodBciEntry * p = (MethodBciEntry*)hashtable->bucket(i);
    for (; p != NULL; p = p->next()) {
      ngen_t * arr = p->literal()->array();
      long * target_gen = p->literal()->target_gen_addr();
      int j = 0;
      int max_idx = 0;
      // careful with overflowing
      while (j < NG2C_GEN_ARRAY_SIZE - 1) {
        if (arr[j] < arr[j+1])
          max_idx = j+1;
        j++;
      }
#ifdef DEBUG_NG2C_PROF_VMOP
      // Must be done prior to restarting the arr.
      gclog_or_tty->print("[ng2c-vmop] <updating target-gen> [");
      for (int k = 0; k < NG2C_GEN_ARRAY_SIZE; k++)
        gclog_or_tty->print(INT64_FORMAT "; ", arr[k]);
      gclog_or_tty->print_cr("] target_gen = " INT32_FORMAT,
                             max_idx > *target_gen ? (*target_gen) + 1 : *target_gen);
#endif
      if (max_idx > *target_gen) {
        (*target_gen)++; // TODO - atomic? volatile?
        // Note: If we decide to change the target gen, we should clear the
        // ngen array. This is necessary because we need to know how many
        // objects (already allocated in the target gen) still survivo a
        // collection.
        memset(arr, 0, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));
      }
    }
  }
}

bool
NG2C_MergeAllocCounters::doit_prologue()
{
  return true;
}
