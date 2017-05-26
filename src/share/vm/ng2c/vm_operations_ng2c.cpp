#include "ng2c/vm_operations_ng2c.hpp"
#include "gc_implementation/g1/g1CollectedHeap.inline.hpp"
#include "gc_implementation/g1/g1CollectorPolicy.hpp"

uint   NG2C_MergeAllocCounters::_total_update_target_gen = 0;
volatile jlong NG2C_MergeAllocCounters::_next_gen = 1;

void
NG2C_MergeWorkerThreads::do_thread(Thread * thread)
{
  if (!thread->is_Named_thread()) return;
  _op->update_promotions((NamedThread*)thread);
}


void
NG2C_MergeAllocCounters::update_promotions(NGenerationArray * global, NGenerationArray * survivors)
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
  MethodBciHashtable * hashtable = thread->method_bci_hashtable();
  MethodBciHashtable * global_hashtable = Universe::method_bci_hashtable();

  for (int i = 0; i < hashtable->table_size(); i++) {
    MethodBciEntry * surv = (MethodBciEntry*)hashtable->bucket(i);
    for (; surv != NULL; surv = surv->next()) {
      NGenerationArray * surv_arr = surv->literal();
      uint hash = surv_arr->hash();
      NGenerationArray * glbl_arr = global_hashtable->get_entry(hash);

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

void
NG2C_MergeAllocCounters::update_target_gen()
{
  MethodBciHashtable * hashtable = Universe::method_bci_hashtable();
  unsigned int cur_tenuring_threshold = ((G1CollectedHeap*)Universe::heap())->g1_policy()->tenuring_threshold();

#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> cur_tenuring_threshold=%u",
           cur_tenuring_threshold);
#endif

  for (int i = 0; i < hashtable->table_size(); i++) {
    MethodBciEntry * p = (MethodBciEntry*)hashtable->bucket(i);

    for (; p != NULL; p = p->next()) {
      NGenerationArray * ngen_arr = p->literal();
      volatile long * target_gen = ngen_arr->target_gen_addr();
      long promo_counter = 0;
      //for (unsigned int i = 1; i < NG2CUpdateThreshold; i++) promo_counter += ngen_arr->array()[i];
      promo_counter = ngen_arr->array()[cur_tenuring_threshold];
      long alloc_counter = ngen_arr->array()[0];

       // TODO - if we exceed  NG2C_GEN_ARRAY_SIZE, then we need to create a new gen
       // TODO - make this 50 a constant somewhere!
      if (/*cur_tenuring_threshold > 1 &&*/ alloc_counter > 50 && promo_counter > alloc_counter * NG2CPromotionThreshold) {
#ifdef NG2C_PROF_ALLOC
#ifdef LAP
        if (*target_gen == 0) {
          Atomic::store(_next_gen + 1 >= NG2C_GEN_ARRAY_SIZE ? NG2C_GEN_ARRAY_SIZE - 1: _next_gen++,
                        (volatile jlong *)target_gen);
        }
#else
        Atomic::inc((volatile jint *)target_gen);
#endif
#endif

#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> hash=%u target_gen=%u",
           ngen_arr->hash(), *target_gen);
#endif
      }
      // Note: we clean the arry to ensure that we look at a single time window.
      memset(ngen_arr->array(), 0, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));
    }
  }
}

bool
NG2C_MergeAllocCounters::doit_prologue()
{
  return true;
}
