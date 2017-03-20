#ifndef SHARE_VM_NG2C_NG2C_GLOBALS_HPP
#define SHARE_VM_NG2C_NG2C_GLOBALS_HPP

# include "memory/allocation.inline.hpp"

typedef unsigned long ngen_t;

const static int NG2C_GEN_ARRAY_SIZE = 16;

class NGenerationArray : public CHeapObj<mtGC>
{
 private:
  // The hash value use is for helping reference the arrays in the hashtable
  // when applying deltas.
  uint     _hash;
  long     _target_gen;
  // The actual array.
  ngen_t * _array;

 public:
  NGenerationArray(uint hash) : _hash(hash), _target_gen(0) {
      _array = NEW_C_HEAP_ARRAY(ngen_t, NG2C_GEN_ARRAY_SIZE, mtGC);
      bzero(_array, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));
  }
  NGenerationArray(ngen_t * array, uint hash, char target_gen) : _array(array), _hash(hash), _target_gen(target_gen) { }
  NGenerationArray(const NGenerationArray& copy) : _array(copy.array()), _hash(copy.hash()), _target_gen(copy.target_gen()) { }

  ngen_t * array() const { return _array; }
  uint     hash()  const { return _hash; }
  int      size()  const { return NG2C_GEN_ARRAY_SIZE; } // hard-coded for now
  ngen_t   at(int pos) const { return _array[pos]; }
  long   target_gen() const { return _target_gen; }
  long * target_gen_addr()  { return &_target_gen; }

  void   apply_delta (NGenerationArray * thread_arr);
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for NGenerationArray...");
    return (unsigned int)0;
  } // placeholder
};

class JavaLocalNGenPair : public CHeapObj<mtGC>
{
 private:
  // The hash value is used to prevent the loss of 1-1 correspondence between hash and index in
  // the thread-local table.
  uint _hash;
  long _target_gen_count;

 public:
  JavaLocalNGenPair(uint hash) : _hash(hash) { }
  JavaLocalNGenPair(const JavaLocalNGenPair& copy) :
    _hash(copy.hash()), _target_gen_count(copy.target_count()) { }

  uint   hash()         const { return _hash; }
  long   target_count() const { return _target_gen_count; }
  long * target_count_addr()  { return &_target_gen_count;}

  // Called by C2 to get the offset of the field the JavaThread will update every allocation
  static ByteSize target_gen_offset()
    { return byte_offset_of(JavaLocalNGenPair, _target_gen_count); }

  // Called after applying deltas during safepoint.
  void reset() { _target_gen_count = 0; }
};

#endif // SHARE_VM_NG2C_NG2C_GLOBALS_HPP
