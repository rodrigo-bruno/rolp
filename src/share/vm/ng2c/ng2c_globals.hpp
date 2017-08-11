#ifndef SHARE_VM_NG2C_NG2C_GLOBALS_HPP
#define SHARE_VM_NG2C_NG2C_GLOBALS_HPP

# include "memory/allocation.inline.hpp"

// TODO - missing stuff. We need to reset the thread's context summary periodically.
// Exceptions and other things like that can easily break the context and therefore
// allocates are out of track. I saw that hapenning in cassandra!
typedef unsigned long ngen_t;

// TODO - NG2C_GEN_ARRAY_SIZE is not necessary. If I only increase or reduce
// the target gen by one unit, then I only need to know if objects allocated
// (in the current target gen) still survive collections. This means that I
// should increase the target gen. In other words, I don't need to track the
// age of an object. I only need to know that it was allocated in a specific gen
// and if it survived or not to collections. Think about it!
const static unsigned int NG2C_GEN_ARRAY_SIZE = 16; // TODO FIXME: Why does it crash with 4? -> this should depend on the current tenuring treshold!
// 2^16, which is the number of possible alloc site ids and context ids
const static unsigned int NG2C_MAX_ALLOC_SITE = 65536; 

class ContextIndex : public CHeapObj<mtGC>
{
 private:
  uint _index;

 public:
  ContextIndex(uint index) : _index(index) { }
  int index() { return _index; }
  // Necessary to used in hashtables.
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for ContextIndex...");
    return (unsigned int)0;
  }

};

class NGenerationArray : public CHeapObj<mtGC>
{
 private:

  // The identifier of the allocation site described by this object.
  uint _hash;

  // The factor can have two values: '0' and '1'. It basicaly controls how
  // methods further below use the '_contexts' array.
  int _factor;

  // Same as the previous one byt in bytes (sizeof(ngen_t)).
  int _factor_bytes;

  // _target_gen[contextID] contains the target generation
  volatile ngen_t * _target_gen;

  // _allocs_gen[contextID] contains the number of allocated objects
  volatile ngen_t * _allocs_gen;

 public:
  NGenerationArray(uint hash) : _hash(hash), _factor(0), _factor_bytes(0) {
// TODO - the commented code is the correct one. However, it brings an issue.
// The jitted code assumes that the _target_gen and _allocs_gen is a constant
// (that will not change). However, when contexts are expanded, the address of
// both variables will change!! We need to add another indirection level. Doing
// this way will bring a ~500K mem overhead for each tracked allocation site!
/*
    _target_gen = NEW_C_HEAP_ARRAY(ngen_t, 1, mtGC);
    memset((void*)_target_gen, 0, 1 * sizeof(ngen_t));
    _allocs_gen = NEW_C_HEAP_ARRAY(ngen_t, 1, mtGC);
    memset((void*)_allocs_gen, 0, 1 * sizeof(ngen_t));
*/
    _target_gen = NEW_C_HEAP_ARRAY(ngen_t, NG2C_MAX_ALLOC_SITE, mtGC);
    memset((void*)_target_gen, 0, NG2C_MAX_ALLOC_SITE * sizeof(ngen_t));
    _allocs_gen = NEW_C_HEAP_ARRAY(ngen_t, NG2C_MAX_ALLOC_SITE, mtGC);
    memset((void*)_allocs_gen, 0, NG2C_MAX_ALLOC_SITE * sizeof(ngen_t));
  }

  uint     hash()  const { return _hash; }
  int      size()  const { return NG2C_GEN_ARRAY_SIZE; } // hard-coded for now
  int *    factor_bytes_addr()  { return &_factor_bytes; }
  ngen_t * gen_addr() { return (ngen_t*)_target_gen; } // TODO - rename to target_addr
  ngen_t * acc_addr() { return (ngen_t*)_allocs_gen; } // TODO - rename to allocs_addr

  void     prepare_contexts();
  bool     expanded_contexts() { return _factor > 0; }

  void     inc_target_gen(unsigned int context);
  long     target_gen(unsigned int context);

  ngen_t   number_allocs(unsigned int context);
  void     inc_number_allocs(unsigned int context); // TODO - not used.
  void     reset_allocs(unsigned int context);

  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for NGenerationArray...");
    return (unsigned int)0;
  } // placeholder
};

class PromotionCounter : public CHeapObj<mtGC>
{
 private:
  ngen_t * _array;
  unsigned int _hash;
  NGenerationArray * _allocations;

 public:
  PromotionCounter(unsigned int hash) :
      _hash(hash), _allocations(NULL) {
    _array = NEW_C_HEAP_ARRAY(ngen_t, NG2C_GEN_ARRAY_SIZE, mtGC);
    memset(_array, 0, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));
  }

  ngen_t * array() const { return _array; }
  void   update(unsigned int age);
  unsigned int hash() { return _hash; }
  // TODO - this method is not being used. It could be used in ng2c vm ops though.
  NGenerationArray * get_allocations();
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for PromotionCounter...");
    return (unsigned int)0;
  }
};

#endif // SHARE_VM_NG2C_NG2C_GLOBALS_HPP
