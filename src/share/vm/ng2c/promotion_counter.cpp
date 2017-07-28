# include "ng2c/promotion_counter.hpp"
# include "memory/nogc.h"

PromotionCounter *
PromotionCounters::add_counter(unsigned int hash)
{
  PromotionCounter * pc = new PromotionCounter(hash);
  HashtableEntry<PromotionCounter *, mtGC> * entry = _counters->new_entry(hash, pc);

  assert(entry != NULL, "promotion counters could not add new entry to hashmap");

  _counters->add_entry(_counters->hash_to_index(hash), entry);

  return pc;
}

PromotionCounter *
PromotionCounters::get_counter(unsigned int hash)
{
  HashtableEntry<PromotionCounter *, mtGC> * entry =
      _counters->bucket(_counters->hash_to_index(hash));

  if (entry == NULL) return NULL;

  while (entry->next() != NULL && entry->hash() != hash) entry = entry->next();

  if (entry->hash() == hash) return entry->literal();

  return NULL;
}

PromotionCounter *
PromotionCounters::get_counter_not_null(unsigned int hash)
{
  HashtableEntry<PromotionCounter *, mtGC> * entry =
      _counters->bucket(_counters->hash_to_index(hash));

  if (entry == NULL) return add_counter(hash);

  while (entry->next() != NULL && entry->hash() != hash) entry = entry->next();

  if (entry->hash() == hash) return entry->literal();
  else return add_counter(hash);
}

void
PromotionCounters::print_on(outputStream * st, const char * tag)
{
  // TODO - implement!
}
