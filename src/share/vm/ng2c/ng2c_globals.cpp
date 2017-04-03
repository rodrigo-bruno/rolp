# include "ng2c/ng2c_globals.hpp"

void
NGenerationArray::update(uint age)
{
  assert(age > 0, "update age should be > 0");
  // TODO - this is not correct, it should be target_gen - age?
  _array[age]++;
}
