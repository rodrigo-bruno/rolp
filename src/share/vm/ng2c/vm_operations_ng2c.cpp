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
  // TODO - global array should have only one entry per alloc site id if the context is not expanded.
  PromotionCounters * hashtable = thread->promotion_counters();
  PromotionCounters * global_hashtable = Universe::promotion_counters();

  for (int i = 0; i < hashtable->get_counters()->table_size(); i++) {
    HashtableEntry<PromotionCounter *, mtInternal> * surv = (HashtableEntry<PromotionCounter *, mtInternal>*)hashtable->get_counters()->bucket(i);
    for (; surv != NULL; surv = surv->next()) {
      PromotionCounter * surv_arr = surv->literal();
      uint hash = surv_arr->hash();
      PromotionCounter * glbl_arr = global_hashtable->get_counter_not_null(hash);

/*
#ifdef DEBUG_NG2C_PROF_VMOP // TODO - necessary?
      for (unsigned int i = 0; i < NG2C_GEN_ARRAY_SIZE; i++)
        if (surv_arr->array()[i])
          gclog_or_tty->print_cr("[ng2c-vmop] <promotions> %s hash="INTPTR_FORMAT" age=%d promotions=%lu",
             glbl_arr == NULL ? "unkown" : "", hash, i, surv_arr->array()[i]);
#endif
*/
      update_promotions(glbl_arr, surv_arr);
    }
  }
}

bool
NG2C_MergeAllocCounters::should_use_context(PromotionCounter * pc)
{
  unsigned int cur_tenuring_threshold = ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();
  long promo_counter = 0;
  promo_counter = pc->array()[cur_tenuring_threshold];
  long alloc_counter = pc->array()[0];
  bool above = promo_counter > alloc_counter * NG2CGenContextThreshold; 
  bool below = promo_counter < alloc_counter * NG2CGenUpdateThreshold;

  if (cur_tenuring_threshold >= 1 && alloc_counter > 50 && above && below) {
    return true;
  }
  return false;
}

bool
NG2C_MergeAllocCounters::should_inc_gen(PromotionCounter * pc)
{
  unsigned int cur_tenuring_threshold = ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();
  long promo_counter = 0;
  promo_counter = pc->array()[cur_tenuring_threshold];
  long alloc_counter = pc->array()[0];
  bool above = promo_counter > alloc_counter * NG2CGenUpdateThreshold;

  if (cur_tenuring_threshold >= 1 && alloc_counter > 50 && above) {
    return true;
  }
  return false;
}

void
NG2C_MergeAllocCounters::update_target_gen()
{
  PromotionCounters * hashtable = Universe::promotion_counters();
  unsigned int cur_tenuring_threshold = get_cur_tenuring_threshold();

  for (int i = 0; i < hashtable->get_counters()->table_size(); i++) {
    HashtableEntry<PromotionCounter *, mtInternal> * p = (HashtableEntry<PromotionCounter *, mtInternal>*)hashtable->get_counters()->bucket(i);

    for (; p != NULL; p = p->next()) {
      PromotionCounter * ngen_arr = p->literal();
      unsigned int context = mask_bits ((uintptr_t)ngen_arr->hash(), 0xFFFF);
      unsigned int alloc_site_id = ((unsigned int)ngen_arr->hash()) >> 16;
      NGenerationArray * allocs = Universe::method_bci_hashtable()->get_entry(alloc_site_id);

      ngen_arr->array()[0] = allocs->number_allocs(context);

      if (!allocs->expanded_contexts() && should_use_context(ngen_arr)) {
#ifdef NG2C_PROF_CONTEXT
        allocs->prepare_contexts();
#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <expanding contexts> hash="INTPTR_FORMAT" target_gen=%u",
           ngen_arr->hash(), allocs->target_gen(context));
#endif
#endif
      }
      else if (should_inc_gen(ngen_arr)) {
#ifdef NG2C_PROF_ALLOC
        allocs->inc_target_gen(context);
#endif
#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> hash="INTPTR_FORMAT" target_gen=%u",
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


unsigned int
NG2C_MergeAllocCounters::get_cur_tenuring_threshold()
{
  return ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();
}
