# include "ng2c/method_bci_hashtable.hpp"
# include "classfile/altHashing.hpp"
# include "memory/nogc.h"
# include "oops/method.hpp"

// TODO - check the size of the table
MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<NGenerationArray*, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

unsigned int
MethodBciHashtable::add_entry(uint hash)
{
  // If we already have this hash inserted, just return.
  if (get_entry(hash)) {
    return hash;
  }

  NGenerationArray * array = new NGenerationArray(hash);
  MethodBciEntry * entry =
    (MethodBciEntry*)Hashtable<NGenerationArray*, mtGC>::new_entry(hash, array);

  assert(entry != NULL, "new entry returned NULL");

  Hashtable<NGenerationArray*, mtGC>::add_entry((hash_to_index(hash)), entry);

#ifdef DEBUG_NG2C_PROF_TABLE
  gclog_or_tty->print_cr("[ng2c-prof-table] add_entry hash="INTPTR_FORMAT" hash_to_index=%d, bucket="INTPTR_FORMAT,
    (intptr_t)hash, hash_to_index(hash), bucket(hash_to_index(hash)));
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

void
MethodBciHashtable::print_on(outputStream * st, const char * tag)
{
  for (int i = 0; i < table_size(); i++) {
    MethodBciEntry * p = (MethodBciEntry*)bucket(i);

    for (; p != NULL; p = p->next()) {
      NGenerationArray * arr = p->literal();
      // TODO - implement print_on for MethodBciHashtable
    }
  }
}
