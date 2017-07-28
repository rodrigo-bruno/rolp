#ifndef SHARE_VM_NG2C_NG2C_GLOBALS_HPP
#define SHARE_VM_NG2C_NG2C_GLOBALS_HPP

# include "memory/allocation.inline.hpp"

typedef unsigned long ngen_t;

// TODO - both these constants should be overriden at launch time!
// TODO - NG2C_GEN_ARRAY_SIZE is not necessary. If I only increase or reduce
// the target gen by one unit, then I only need to know if objects allocated
// (in the current target gen) still survive collections. This means that I
// should increase the target gen. In other words, I don't need to track the
// age of an object. I only need to know that it was allocated in a specific gen
// and if it survived or not to collections. Think about it!
const static unsigned int NG2C_GEN_ARRAY_SIZE = 16; // TODO FIXME: Why does it crash with 4?
const static unsigned int NG2C_MAX_ALLOC_SITE = 1024*1024; // TODO - change this to 2^16

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

  // The generation that should be used for allocations taking place at the
  // allocation site identified by the '_hash' and that require no context
  // profiling.
  volatile long _target_gen;

  // The actual number of objects allocated in the target gen.
  ngen_t _counter;

  // Contain information for fine grained allocation context profiling.
  // *(_contexts[contextID * 2]) contains the target generation.
  // *(_contexts[contextID * 2 + 1]) contains the number of allocated objects.
  ngen_t * _contexts;


 public:
  NGenerationArray(uint hash) :
      _hash(hash), _target_gen(0), _counter(0), _contexts(NULL) { }

  uint     hash()  const { return _hash; }
  int      size()  const { return NG2C_GEN_ARRAY_SIZE; } // hard-coded for now

  void     prepare_contexts();
  ngen_t * contexts() { return _contexts; }

  void     inc_target_gen(unsigned int context);
  long     target_gen(unsigned int context);

  ngen_t   number_allocs(unsigned int context);
  void     inc_number_allocs(unsigned int context);
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
  NGenerationArray * get_allocations();
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for PromotionCounter...");
    return (unsigned int)0;
  }
};

#endif // SHARE_VM_NG2C_NG2C_GLOBALS_HPP
