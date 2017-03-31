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

  if (get_entry(hash) != NULL)
    goto Return;

  Hashtable<NGenerationArray*, mtGC>::add_entry((hash_to_index(hash)), entry);

#ifdef DEBUG_NG2C_PROF_TABLE
  gclog_or_tty->print_cr("[ng2c-prof-table] add_entry(method="INTPTR_FORMAT", bci=%d) -> [hash="INTPTR_FORMAT" hash_to_index=%d, bucket="INTPTR_FORMAT,
                         m, bci, (intptr_t)hash, hash_to_index(hash), bucket(hash_to_index(hash)));
#endif

Return:
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

  if (entry == NULL) return NULL;

  while (entry->next() != NULL && entry->hash() != hash) entry = entry->next();

  if (entry->hash() == hash) return entry->literal();

  return NULL;
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

ngen_t *
MethodBciHashtable::get_alloc_slot(uint hash)
{
  assert (hash != 0, "hash cannot be 0 since it was previously computed.");

  NGenerationArray * arr = get_entry(hash);

  assert (arr != NULL, "arr cannot be NULL.");

  return arr->array();
}

volatile long *
MethodBciHashtable::get_target_gen(uint hash)
{
  if (hash == 0) return NULL;

  NGenerationArray * arr = get_entry(hash);

  assert(arr != NULL, "get entry returned NULL");

  return arr->target_gen_addr();
}

unsigned int
MethodBciHashtable::calculate_hash(Method * m, int bci)
{
  unsigned int hash = (unsigned int)AltHashing::murmur3_32(bci, (const jbyte*)m, sizeof(Method));
  return hash;
}

/* printing */
void
MethodBciHashtable::print_on(outputStream * st, const char * tag)
{
  // TODO: Should we implement a kind of keySet here to
  // speedup this loop and other's similar?
  for (int i = 0; i < table_size(); i++) {
    MethodBciEntry * p = (MethodBciEntry*)bucket(i);

    for (; p != NULL; p = p->next()) {
      ngen_t * arr = p->literal()->array();
      volatile long * target_gen = p->literal()->target_gen_addr();
      st->print("[ng2c-vmop] <%s> hash=%u target_gen=%u [",
                tag, p->literal()->hash(), *target_gen);

      for (int k = 0; k < NG2C_GEN_ARRAY_SIZE; k++)
        st->print(INT64_FORMAT "; ", arr[k]);

      st->print_cr("]");
    }
  }
}
