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

      update_promotions(glbl_arr, surv_arr);
    }
  }
}

int
NG2C_MergeAllocCounters::normalize_derive_analyze(PromotionCounter * pc)
{
    float normalized[NG2C_GEN_ARRAY_SIZE] = {0};
    float derivative[NG2C_GEN_ARRAY_SIZE] = {0};
		unsigned long * curve = pc->array();
    unsigned int segm = NG2C_GEN_ARRAY_SIZE;
    unsigned int min = 0;

    // Normalize
    for (unsigned int i = 0; i < NG2C_GEN_ARRAY_SIZE - 1; i++) {
        if (curve[i]  != 0) {
            normalized[i] = (float) curve[i] / curve[0];
        }
    }

    // Derive
    for (unsigned int i = 0; i < NG2C_GEN_ARRAY_SIZE - 1; i++) {
        derivative[i] = normalized[i + 1] - normalized[i];
        if (derivative[i] > -0.1) {
            derivative[i] = 0;
        }
    }

    // Analyze
    for (unsigned int i = 0; i < NG2C_GEN_ARRAY_SIZE; i++) {
        if (derivative[i] != 0) {
						// If it is not a consecutive segment and if a previous segment
						// was already found
            if (i != segm + 1 && segm != NG2C_GEN_ARRAY_SIZE) {
                return -1; // Conflict!
            } else {
                segm = i;
            }
            if (derivative[i] < derivative[min]) {
                min = i;
            }
        }
    }
    return min;
}

void
NG2C_MergeAllocCounters::update_target_gen()
{
  PromotionCounters * hashtable = Universe::promotion_counters();

  for (int i = 0; i < hashtable->get_counters()->table_size(); i++) {
    HashtableEntry<PromotionCounter *, mtInternal> * p = (HashtableEntry<PromotionCounter *, mtInternal>*)hashtable->get_counters()->bucket(i);

    for (; p != NULL; p = p->next()) {
      PromotionCounter * ngen_arr = p->literal();
      int decision = 0;
      unsigned int context = mask_bits ((uintptr_t)ngen_arr->hash(), 0xFFFF);
      unsigned int alloc_site_id = ((unsigned int)ngen_arr->hash()) >> 16;
      NGenerationArray * allocs = Universe::method_bci_hashtable()->get_entry(alloc_site_id);
      assert(allocs != NULL, "there should be an ngen array for this");

      // Update promotions array with the number of allocs.
      ngen_arr->array()[0] = allocs->number_allocs(context);

      decision = normalize_derive_analyze(ngen_arr);

      if (decision > 0) {
#ifdef NG2C_PROF_ALLOC
        allocs->inc_target_gen(context);
#endif
#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> hash="INTPTR_FORMAT" decision=%d target_gen=%u",
           ngen_arr->hash(), decision, allocs->target_gen(context));
#endif
      }
      else if (!allocs->expanded_contexts() && decision < 0) {
#ifdef NG2C_PROF_CONTEXT
        allocs->prepare_contexts();
#endif
#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <expanding contexts> hash="INTPTR_FORMAT" decision=%d target_gen=%u",
           ngen_arr->hash(), decision, allocs->target_gen(context));
#endif
      }
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
