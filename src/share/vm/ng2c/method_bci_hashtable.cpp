# include "ng2c/method_bci_hashtable.hpp"
# include "classfile/altHashing.hpp"
# include "memory/nogc.h"
# include "oops/method.hpp"

MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<NGenerationArray*, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

unsigned int
MethodBciHashtable::add_entry(Method * m, int bci)
{
  unsigned int hash = calculate_hash(m, bci);
  unsigned int rhash = mask_bits ((uintptr_t)hash, 0x1FFFFFF);
  return add_entry(rhash);
}

unsigned int
MethodBciHashtable::add_entry(uint hash)
{
  NGenerationArray * array = new NGenerationArray(hash);
  MethodBciEntry * entry =
    (MethodBciEntry*)Hashtable<NGenerationArray*, mtGC>::new_entry(hash, array);

  assert(entry != NULL, "new entry returned NULL");

  Hashtable<NGenerationArray*, mtGC>::add_entry((hash_to_index(hash)), entry);

#ifdef DEBUG_NG2C_PROF_TABLE
  gclog_or_tty->print_cr("[ng2c-prof-table] add_entry(method="INTPTR_FORMAT", bci=%d) -> [hash="INTPTR_FORMAT" hash_to_index=%d, bucket="INTPTR_FORMAT,
    m, bci, (intptr_t)hash, hash_to_index(hash), bucket(hash_to_index(hash)));
#endif

  return hash;
}

NGenerationArray *
MethodBciHashtable::get_entry(uint hash)
{
  if (hash == 0) return NULL;

  int idx = hash_to_index(hash);
  MethodBciEntry * entry = (MethodBciEntry*)bucket(idx);

#ifdef DEBUG_NG2C_PROF_TABLE
  gclog_or_tty->print_cr("[ng2c-prof-table] get_entry(hash="INTPTR_FORMAT") -> "INTPTR_FORMAT,
    hash, entry);
#endif

  assert(entry != NULL, "get entry returned NULL");

  while (entry->next() != NULL && entry->hash() != hash) entry = entry->next();

  assert(entry->hash() == hash, "hash not found");

  return entry->literal();
}

NGenerationArray *
MethodBciHashtable::get_entry_not_null(uint hash)
{
  int idx = hash_to_index(hash);
  MethodBciEntry * entry = (MethodBciEntry*)bucket(idx);

  if (entry == NULL) {
      add_entry(hash);
      return ((MethodBciEntry*)bucket(idx))->literal();
  }

  while (entry->next() != NULL && entry->hash() != hash) entry = entry->next();

  if (entry->hash() != hash) {
      add_entry(hash);
      return ((MethodBciEntry*)bucket(idx))->literal();
  }

  return entry->literal();

}

long *
MethodBciHashtable::get_target_gen(uint hash)
{
  if (hash == 0) return NULL;

  NGenerationArray * arr = get_entry(hash);

  assert(arr != NULL, "get entry returned NULL");

  return arr->target_gen_addr();
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
  return hash;
}