# include "ng2c/method_bci_hashtable.hpp"
# include "classfile/altHashing.hpp"
# include "memory/nogc.h"

MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<NGenerationArray*, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

ngen_t
MethodBciHashtable::add_entry(Method * m, int bci)
{
  unsigned int hash = calculate_hash(m, bci);
  unsigned int rhash = mask_bits ((uintptr_t)hash, 0x1FFFFFF);
  NGenerationArray * array = new NGenerationArray(rhash);
  MethodBciEntry * entry =
    (MethodBciEntry*)Hashtable<NGenerationArray*, mtGC>::new_entry(rhash, array);
  // return last element to save on generated method code for faster access
  return array->at(NG2C_GEN_ARRAY_SIZE-1);
}

NGenerationArray *
MethodBciHashtable::get_entry(uint hash)
{
  int idx = hash_to_index(hash);
  MethodBciEntry * entry = (MethodBciEntry*)bucket(idx);
  if (entry->next() != NULL) {
    while (entry->hash() != hash) entry = entry->next();
    return entry->literal();
  } else {
    return NULL;
  }
}

ngen_t *
MethodBciHashtable::get_target_gen(uint hash)
{
  NGenerationArray * arr = get_entry(hash);
  return arr->get_target_gen_addr();
}

void
MethodBciHashtable::apply_delta(NGenerationArray ** gclocal_ngen_arrays, int sz)
{
  assert (sz > 0, "check if it's worth calling this in the first place.");
  
  int idx = 0;
  do {
    NGenerationArray * temp_arr = gclocal_ngen_arrays[idx];
    NGenerationArray * real_arr = get_entry(temp_arr->hash());
    real_arr->apply_delta(temp_arr);
  } while (++idx < sz);
}

unsigned int
MethodBciHashtable::calculate_hash(Method * m, int bci)
{
  unsigned int hash = (unsigned int)AltHashing::murmur3_32(bci, (const jbyte*)m, sizeof(Method));
#if DEBUG_NG2C_PROF
  gclog_or_tty->print_cr("[ng2c-prof] object hash = " INTPTR_FORMAT, (intptr_t)hash);
#endif
  return hash;
}
