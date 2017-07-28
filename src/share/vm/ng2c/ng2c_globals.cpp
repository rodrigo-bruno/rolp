# include "ng2c/ng2c_globals.hpp"

# include "memory/universe.inline.hpp"

void
NGenerationArray::prepare_contexts()
{
  // Note: we use NG2C_GEN_ARRAY_SIZE * 2 because we are allocating memory for
  // the target generation and for the allocation counter.
  _contexts = NEW_C_HEAP_ARRAY(ngen_t, NG2C_GEN_ARRAY_SIZE * 2, mtGC);
  memset(_contexts, 0, (NG2C_GEN_ARRAY_SIZE * 2) * sizeof(ngen_t));
}

void
NGenerationArray::inc_target_gen(unsigned int context)
{
  if (_contexts == NULL)
    Atomic::inc((volatile jint *)&_target_gen);
  else
    Atomic::inc((volatile jint *)&_contexts[context * 2]);
}

ngen_t
NGenerationArray::number_allocs(unsigned int context)
{
  if (_contexts == NULL)
    return _counter;
  else
    return _contexts[context * 2];
}

void
NGenerationArray::inc_number_allocs(unsigned int context)
{
  if (_contexts == NULL)
    _counter++;
  else
    _contexts[context * 2]++;
}
void
NGenerationArray::reset_allocs(unsigned int context)
{
  if (_contexts == NULL)
    _counter = 0;
  else
    _contexts[context * 2] = 0;
}

long
NGenerationArray::target_gen(unsigned int context)
{
  if (_contexts == NULL)
    return _target_gen;
  else
    return _contexts[context * 2 + 1];
}

void
PromotionCounter::update(unsigned int age)
{
  assert(age > 0, "update age should be > 0");
  _array[age >= NG2C_GEN_ARRAY_SIZE ? NG2C_GEN_ARRAY_SIZE - 1 : age]++;
}

NGenerationArray *
PromotionCounter::get_allocations()
{
  unsigned int alloc_site_id = mask_bits ((uintptr_t)_hash, 0xFFFF);
  if (_allocations == NULL) {
    _allocations = Universe::method_bci_hashtable()->get_entry(alloc_site_id);
  }
  return _allocations;
}
