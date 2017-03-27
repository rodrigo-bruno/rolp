# include "ng2c/ng2c_globals.hpp"

// TODO - needed?
void
NGenerationArray::apply_delta(NGenerationArray * thread_arr)
{
  assert(thread_arr->size() == size(), "ngen array size mismatch");
  
  ngen_t * larr = thread_arr->array();
  int max = size();
  int idx = 0;
  do {
    long delta = (long)larr[idx] - (long)_array[idx];
    _array[idx] += delta;
  } while (++idx < max);
}

void
NGenerationArray::update(uint age)
{
  assert(age > 0, "update age should be > 0");
  _array[age]++;
}
