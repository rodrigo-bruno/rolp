#ifndef SHARE_VM_NG2C_NG2C_GLOBALS_HPP
#define SHARE_VM_NG2C_NG2C_GLOBALS_HPP

# include "memory/allocation.inline.hpp"

typedef unsigned long ngen_t;

const static int NG2C_GEN_ARRAY_SIZE = 17;

class NGenerationArray : public CHeapObj<mtGC>
{
 private:
  ngen_t * _array;

 public:
  NGenerationArray() { _array = NEW_C_HEAP_ARRAY(ngen_t, NG2C_GEN_ARRAY_SIZE, mtGC); }
  NGenerationArray(ngen_t * array) : _array(array) { }
  NGenerationArray(const NGenerationArray& copy) : _array(copy.array()) { }

  ngen_t * array() const { return _array; }
  int size() const { return NG2C_GEN_ARRAY_SIZE; } // hard-coded for now
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for NGenerationArray...");
    return (unsigned int)0;
  } // placeholder
  ngen_t at(int pos) const { return _array[pos]; }
};

#endif // SHARE_VM_NG2C_NG2C_GLOBALS_HPP
