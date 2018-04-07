# include "ng2c/ng2c_globals.hpp"

# include "memory/universe.inline.hpp"

void
NGenerationArray::prepare_contexts()
{
/*
  // Note: backup a copy to delete and backup previous values.
  ngen_t * prev_target = (ngen_t*)_target_gen;
  ngen_t * prev_allocs = (ngen_t*)_allocs_gen;

  ngen_t * new_target = NEW_C_HEAP_ARRAY(ngen_t, NG2C_MAX_ALLOC_SITE, mtGC); // TODO - we should be using max arraya size!!
  ngen_t * new_allocs = NEW_C_HEAP_ARRAY(ngen_t, NG2C_MAX_ALLOC_SITE, mtGC);

  memset(new_target, 0, NG2C_MAX_ALLOC_SITE * sizeof(ngen_t));
  memset(new_allocs, 0, NG2C_MAX_ALLOC_SITE * sizeof(ngen_t));

  // Note: copy the only two values that were being used.
  new_target[0] = prev_target[0];
  new_allocs[0] = prev_allocs[0];

  // Note: atomic xchg
  Atomic::xchg_ptr(new_target, &_target_gen);
  Atomic::xchg_ptr(new_allocs, &_allocs_gen);

  // Note: free previous array
  FREE_C_HEAP_ARRAY(ngen_t*, prev_target, mtGC);
  FREE_C_HEAP_ARRAY(ngen_t*, prev_allocs, mtGC);
*/
  // Note: use the new factor
  _factor = 1;
  _factor_bytes = sizeof(ngen_t);
  // Reset values.
  _allocs_gen[0] = 0;
  // Copy current target generation to everyone.
  ngen_t target = _target_gen[0];
  ngen_t* aux = (ngen_t*)_target_gen;
  // Note: memset does not work because we are using long values!
  for (unsigned int i = 0; i < NG2C_MAX_ALLOC_SITE; i++) *aux++ = target;
}

void
NGenerationArray::inc_target_gen(unsigned int context)
{
  assert((context * _factor) < NG2C_MAX_ALLOC_SITE, "index falls outside");
  _target_gen[context * _factor]++;
  assert(_target_gen[context * _factor] >= 0 && _target_gen[context * _factor] < NG2C_GEN_ARRAY_SIZE, "");
}

void
NGenerationArray::inc_target_gen(unsigned int context, unsigned int new_gen)
{
  assert((context * _factor) < NG2C_MAX_ALLOC_SITE, "index falls outside");
  _target_gen[context * _factor] = new_gen;
  assert(_target_gen[context * _factor] >= 0 && _target_gen[context * _factor] < NG2C_GEN_ARRAY_SIZE, "");
}

ngen_t
NGenerationArray::number_allocs(unsigned int context)
{
  assert((context * _factor) < NG2C_MAX_ALLOC_SITE, "index falls outside");
  return _allocs_gen[context * _factor];
}

void
NGenerationArray::inc_number_allocs(unsigned int context)
{
  assert((context * _factor) < NG2C_MAX_ALLOC_SITE, "index falls outside");
  _allocs_gen[context * _factor]++;
}
void
NGenerationArray::reset_allocs(unsigned int context)
{
  assert((context * _factor) < NG2C_MAX_ALLOC_SITE, "index falls outside");
  _allocs_gen[context * _factor] = 0;
}

long
NGenerationArray::target_gen(unsigned int context)
{
  assert((context * _factor) < NG2C_MAX_ALLOC_SITE, "index falls outside");
  long target_gen = _target_gen[context * _factor];
  assert(target_gen >= 0 && target_gen < NG2C_GEN_ARRAY_SIZE, "");
  return target_gen;
}

void
PromotionCounter::update(unsigned int age)
{
  assert(age > 0, "update age should be > 0");
  _array[age >= NG2C_GEN_ARRAY_SIZE ? NG2C_GEN_ARRAY_SIZE - 1 : age]++;
}

long
PromotionCounter::target_gen()
{
  unsigned int alloc_site = _hash >> 16;
  unsigned int context    = mask_bits ((uintptr_t)_hash, 0xFFFF);
  NGenerationArray* allocs = Universe::method_bci_hashtable()->get_entry(alloc_site);
  assert(allocs != NULL, "allocs should not be null");
  return allocs->target_gen(context);
}
