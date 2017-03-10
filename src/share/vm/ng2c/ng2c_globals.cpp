# include "ng2c/ng2c_globals.hpp"

void
NGenerationArray::apply_delta(NGenerationArray * thread_arr)
{
  assert(thread_arr->size() == size(), "ngen array size mismatch");
  
  ngen_t * larr = thread_arr->array();
  int idx = 0;
  do {
    long delta = (long)larr[idx] - (long)_array[idx];
    _array[idx] += delta;
  } while (++idx < larr->size());
}
