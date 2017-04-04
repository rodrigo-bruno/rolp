# include "ng2c/ng2c_globals.hpp"

void
NGenerationArray::update(uint age)
{
  assert(age > 0, "update age should be > 0");
  _array[age >= NG2C_GEN_ARRAY_SIZE ? NG2C_GEN_ARRAY_SIZE - 1 : age]++;
}
