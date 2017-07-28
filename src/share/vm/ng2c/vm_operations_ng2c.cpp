#include "ng2c/vm_operations_ng2c.hpp"
#include "gc_implementation/g1/g1CollectedHeap.inline.hpp"
#include "gc_implementation/g1/g1CollectorPolicy.hpp"

uint   NG2C_MergeAllocCounters::_total_update_target_gen = 0;

void
NG2C_MergeWorkerThreads::do_thread(Thread * thread)
{
  if (!thread->is_Named_thread()) return;
  _op->update_promotions((NamedThread*)thread);
}


void
NG2C_MergeAllocCounters::update_promotions(PromotionCounter * global, PromotionCounter * survivors)
{
  assert (global != NULL, "global array should not be NULL");
  assert (survivors != NULL, "survivors array should not be NULL");
  ngen_t * garr = global->array();
  ngen_t * sarr = survivors->array();

  sarr++;
  garr++;
  for (unsigned int i = 0; i < NG2C_GEN_ARRAY_SIZE - 1; i++) *garr++ += *sarr++;
  memset((void*)survivors->array(), 0, sizeof(ngen_t) * NG2C_GEN_ARRAY_SIZE);
}

void
NG2C_MergeAllocCounters::update_promotions(NamedThread * thread)
{
  PromotionCounters * hashtable = thread->promotion_counters();
  PromotionCounters * global_hashtable = Universe::promotion_counters();

  for (int i = 0; i < hashtable->get_counters()->table_size(); i++) {
    HashtableEntry<PromotionCounter *, mtInternal> * surv = (HashtableEntry<PromotionCounter *, mtInternal>*)hashtable->get_counters()->bucket(i);
    for (; surv != NULL; surv = surv->next()) {
      PromotionCounter * surv_arr = surv->literal();
      uint hash = surv_arr->hash();
      PromotionCounter * glbl_arr = global_hashtable->get_counter(hash);

#ifdef DEBUG_NG2C_PROF_VMOP
      for (unsigned int i = 0; i < NG2C_GEN_ARRAY_SIZE; i++)
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

bool
NG2C_MergeAllocCounters::should_use_context(PromotionCounter * pc)
{
  // TODO - implement should_use_context.
  // - check if we have more than 75% of objects in one slot
  // - check if we have more than X objects
  return false;
}

bool
NG2C_MergeAllocCounters::should_inc_gen(PromotionCounter * pc)
{
  unsigned int cur_tenuring_threshold = ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();
  long promo_counter = 0;
  //for (unsigned int i = 1; i < NG2CUpdateThreshold; i++) promo_counter += ngen_arr->array()[i];
  promo_counter = pc->array()[cur_tenuring_threshold];
  long alloc_counter = pc->array()[0];
  // TODO - improve should_inc_gen
  // - if we exceed  NG2C_GEN_ARRAY_SIZE, then we need to create a new gen
  // - make this 50 a constant somewhere!
  if (cur_tenuring_threshold > 1 && alloc_counter > 50 && promo_counter > alloc_counter * NG2CPromotionThreshold) {
    return true;
  }
  return false;
}

void
NG2C_MergeAllocCounters::update_target_gen()
{
  PromotionCounters * hashtable = Universe::promotion_counters();

#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
  unsigned int cur_tenuring_threshold = ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();
  gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> cur_tenuring_threshold=%u",
        cur_tenuring_threshold);
#endif

  for (int i = 0; i < hashtable->get_counters()->table_size(); i++) {
    HashtableEntry<PromotionCounter *, mtInternal> * p = (HashtableEntry<PromotionCounter *, mtInternal>*)hashtable->get_counters()->bucket(i);

    for (; p != NULL; p = p->next()) {
      PromotionCounter * ngen_arr = p->literal();
      unsigned int context = ((unsigned int)ngen_arr->hash()) >> 16;
      unsigned int alloc_site_id = mask_bits ((uintptr_t)ngen_arr->hash(), 0xFFFF);
      NGenerationArray * allocs = Universe::method_bci_hashtable()->get_entry(alloc_site_id);

      ngen_arr->array()[0] = allocs->number_allocs(context);

      if (context == 0 && should_use_context(ngen_arr)) {
#if defined(NG2C_PROF_ALLOC) && !defined(DISABLE_NG2C_PROF_CONTEXT)
        allocs->prepare_contexts();
#endif
        // TODO - delete entry in the global promotion counter
      }

      if (should_inc_gen(ngen_arr)) {
#ifdef NG2C_PROF_ALLOC
        allocs->inc_target_gen(context);
#endif

#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> hash=%u target_gen=%u",
           ngen_arr->hash(), allocs->target_gen(context));
#endif
      }
      // Note: we clean the arry to ensure that we look at a single time window.
      memset(ngen_arr->array(), 0, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));
      allocs->reset_allocs(context);
    }
  }
}

bool
NG2C_MergeAllocCounters::doit_prologue()
{
  return true;
}
