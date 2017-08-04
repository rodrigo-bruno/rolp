# include "ng2c/method_bci_hashtable.hpp"
# include "classfile/altHashing.hpp"
# include "memory/nogc.h"
# include "oops/method.hpp"

#include<pthread.h>

// This is required to serialize insertions into the hashtable.
// This should be done properly by using internal locks (and not directly mutexes...)
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 

MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<NGenerationArray*, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

NGenerationArray *
MethodBciHashtable::add_entry(uint hash)
{
  pthread_mutex_lock(&lock);
  NGenerationArray * array = get_entry(hash);

  // If we already have this hash inserted, just return.
  if (array != NULL) {
    pthread_mutex_unlock(&lock);
    return array;
  }

  array = new NGenerationArray(hash);
  MethodBciEntry * entry =
    (MethodBciEntry*)Hashtable<NGenerationArray*, mtGC>::new_entry(hash, array);

  assert(entry != NULL, "new entry returned NULL");

  Hashtable<NGenerationArray*, mtGC>::add_entry((hash_to_index(hash)), entry);

#ifdef DEBUG_NG2C_PROF_ALLOCS_TABLE
  gclog_or_tty->print_cr("[ng2c-prof-table] add_entry hash="INTPTR_FORMAT" hash_to_index=%d, bucket="INTPTR_FORMAT,
    (intptr_t)hash, hash_to_index(hash), bucket(hash_to_index(hash)));
#endif

  pthread_mutex_unlock(&lock);
  return array;
}

NGenerationArray *
MethodBciHashtable::get_entry(uint hash)
{
  if (hash == 0) return NULL;

  int idx = hash_to_index(hash);
  MethodBciEntry * entry = (MethodBciEntry*)bucket(idx);

#ifdef DEBUG_NG2C_PROF_ALLOCS_TABLE
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
      ngen_t * allocs = p->literal()->acc_addr();
      ngen_t * target = p->literal()->gen_addr();
      uint alloc_site_id = p->literal()->hash();
      unsigned int size = p->literal()->expanded_contexts() ? NG2C_MAX_ALLOC_SITE : 1;

      for (unsigned int j = 0; j < size; j++) {
        if (allocs[j] == 0) continue;
        st->print_cr("[ng2c-%s] alloc_site_id="INTPTR_FORMAT" expanded=%s context="INTPTR_FORMAT" target_gen=%d allocs=%d",
            tag, alloc_site_id, size > 1 ? "true" : "false", j, target[j], allocs[j]);
      }
    }
  }
}
