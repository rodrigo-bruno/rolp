# include "ng2c/ng2c_globals.hpp"

# include "memory/universe.inline.hpp"

void
NGenerationArray::prepare_contexts()
{
  // Note: we use NG2C_GEN_ARRAY_SIZE * 2 because we are allocating memory for
  // the target generation and for the allocation counter.
  ngen_t * preva = (ngen_t*)_contexts;
  ngen_t * newa = NEW_C_HEAP_ARRAY(ngen_t, NG2C_GEN_ARRAY_SIZE * 2, mtGC);
  memset(newa, 0, (NG2C_GEN_ARRAY_SIZE * 2) * sizeof(ngen_t));
  // Note: copy the only two values that were being used.
  newa[0] = _contexts[0];
  newa[1] = _contexts[1];
  // Note: atomic xchg
  Atomic::xchg_ptr(newa, _contexts);
  // Note: free previous array
  FREE_C_HEAP_ARRAY(ngen_t*, preva, mtGC);
  // Note: use the new factor
  _factor = 2;
}

void
NGenerationArray::inc_target_gen(unsigned int context)
{
  Atomic::inc((volatile jint *)&_contexts[context * _factor]);
}

ngen_t
NGenerationArray::number_allocs(unsigned int context)
{
  return _contexts[context * _factor];
}

void
NGenerationArray::inc_number_allocs(unsigned int context)
{
  _contexts[context * _factor]++;
}
void
NGenerationArray::reset_allocs(unsigned int context)
{
  _contexts[context * _factor] = 0;
}

long
NGenerationArray::target_gen(unsigned int context)
{
  return _contexts[context * _factor + 1];
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
