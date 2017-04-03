# include "ng2c/vm_operations_ng2c.hpp"

uint   NG2C_MergeAllocCounters::_total_update_target_gen = 0;

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
  for (int i = 0; i < NG2C_GEN_ARRAY_SIZE - 1; i++) *garr++ += *sarr++;
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
NG2C_MergeAllocCounters::update_target_gen()
{
  MethodBciHashtable * hashtable = Universe::method_bci_hashtable();

  for (int i = 0; i < hashtable->table_size(); i++) {
    MethodBciEntry * p = (MethodBciEntry*)hashtable->bucket(i);

    for (; p != NULL; p = p->next()) {
      ngen_t * arr = p->literal()->array();
      ngen_t * sav = arr;
      volatile long * target_gen = p->literal()->target_gen_addr();
      long promo_counter = 0;

      for (int j = 1; j < NG2C_GEN_ARRAY_SIZE; j++) promo_counter += *++arr;
        // TODO - if we exceed  NG2C_GEN_ARRAY_SIZE, then we need to create a new gen
        // TODO - replace .5 with constant (defined at launch time!)
      if (promo_counter > *sav * .5) {
#ifdef NG2C_PROF_ALLOC
        Atomic::inc((volatile jint *)target_gen);
#endif
        // Note: If we decide to change the target gen, we should clear the
        // ngen array. This is necessary because we need to know how many
        // objects (already allocated in the target gen) still survivo a
        // collection.
        memset(sav, 0, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));

#if defined(DEBUG_NG2C_PROF_VMOP) || defined(DEBUG_NG2C_PROF_VMOP_UPDATE)
        gclog_or_tty->print_cr("[ng2c-vmop] <updating target-gen> hash=%u target_gen=%u",
           p->literal()->hash(), *target_gen);
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
