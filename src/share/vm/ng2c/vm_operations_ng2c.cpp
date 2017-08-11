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
      unsigned int alloc_site_id = hash >> 16;
      NGenerationArray * ngen = Universe::method_bci_hashtable()->get_entry(alloc_site_id);
      assert(ngen != NULL, "there should be an ngen array for this");

      // Note: for allocation sites that are expanded, we must use the hash.
      // For others, we should only use the alloc site id with a zeroed context.
      // See g1CollectedHeap.cpp.
      PromotionCounter * glbl_arr = ngen->expanded_contexts() ?
          global_hashtable->get_counter_not_null(hash) :
          global_hashtable->get_counter_not_null(alloc_site_id << 16);

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
  cur_tenuring_threshold = NG2CUpdateThreshold > cur_tenuring_threshold ?
      cur_tenuring_threshold : NG2CUpdateThreshold;
  long promo_counter = 0;
  promo_counter = pc->array()[cur_tenuring_threshold - 1];
  long alloc_counter = pc->array()[0];

  long factor = alloc_counter;
  for (unsigned int i = 0; i < cur_tenuring_threshold; i++) {
    factor = factor * NG2CGenContextThreshold;
  }

  bool above = promo_counter > factor; 

  if (cur_tenuring_threshold >= 1 && alloc_counter > 50 && above) {
    return true;
  }
  return false;
}

bool
NG2C_MergeAllocCounters::should_inc_gen(PromotionCounter * pc)
{
  unsigned int cur_tenuring_threshold = ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();
  cur_tenuring_threshold = NG2CUpdateThreshold > cur_tenuring_threshold ?
      cur_tenuring_threshold : NG2CUpdateThreshold;

  long promo_counter = 0;
  promo_counter = pc->array()[cur_tenuring_threshold - 1];
  long alloc_counter = pc->array()[0];

  long factor = alloc_counter;
  for (unsigned int i = 0; i < cur_tenuring_threshold; i++) {
    factor = factor * NG2CGenUpdateThreshold;
  }

  bool above = promo_counter > factor;

  if (cur_tenuring_threshold >= 1 && alloc_counter > 50 && above) {
    return true;
  }
  return false;
}

void
NG2C_MergeAllocCounters::update_target_gen()
{
  PromotionCounters * hashtable = Universe::promotion_counters();

  for (int i = 0; i < hashtable->get_counters()->table_size(); i++) {
    HashtableEntry<PromotionCounter *, mtInternal> * p = (HashtableEntry<PromotionCounter *, mtInternal>*)hashtable->get_counters()->bucket(i);

    for (; p != NULL; p = p->next()) {
      PromotionCounter * ngen_arr = p->literal();
      unsigned int context = mask_bits ((uintptr_t)ngen_arr->hash(), 0xFFFF);
      unsigned int alloc_site_id = ((unsigned int)ngen_arr->hash()) >> 16;
      NGenerationArray * allocs = Universe::method_bci_hashtable()->get_entry(alloc_site_id);
      assert(allocs != NULL, "there should be an ngen array for this");

      // Update promotions array with the number of allocs.
      ngen_arr->array()[0] = allocs->number_allocs(context);

      if (should_inc_gen(ngen_arr)) {
#ifdef NG2C_PROF_ALLOC
        allocs->inc_target_gen(context);
#endif
#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> hash="INTPTR_FORMAT" target_gen=%u",
           ngen_arr->hash(), allocs->target_gen(context));
#endif
      }
      else if (!allocs->expanded_contexts() && should_use_context(ngen_arr)) {
#ifdef NG2C_PROF_CONTEXT
        allocs->prepare_contexts();
#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <expanding contexts> hash="INTPTR_FORMAT" target_gen=%u",
           ngen_arr->hash(), allocs->target_gen(context));
#endif
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
