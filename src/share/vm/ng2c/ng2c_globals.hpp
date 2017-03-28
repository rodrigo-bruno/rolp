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
const static int NG2C_GEN_ARRAY_SIZE = 16;
const static int NG2C_MAX_ALLOC_SITE = 1024*1024;

class NGenerationArray : public CHeapObj<mtGC>
{
 private:
  // The hash value use is for helping reference the arrays in the hashtable
  // when applying deltas.
  uint     _hash;
  volatile long     _target_gen;
  // The actual array.
  ngen_t * _array;

 public:
  NGenerationArray(uint hash) : _hash(hash), _target_gen(0) {
    _array = NEW_C_HEAP_ARRAY(ngen_t, NG2C_GEN_ARRAY_SIZE, mtGC);
    memset(_array, 0, (NG2C_GEN_ARRAY_SIZE) * sizeof(ngen_t));
  }
  NGenerationArray(ngen_t * array, uint hash, volatile long target_gen) : _array(array), _hash(hash), _target_gen(target_gen) { }
  NGenerationArray(const NGenerationArray& copy) : _array(copy.array()), _hash(copy.hash()), _target_gen(copy.target_gen()) { }

  ngen_t * array() const { return _array; }
  uint     hash()  const { return _hash; }
  int      size()  const { return NG2C_GEN_ARRAY_SIZE; } // hard-coded for now
  ngen_t   at(int pos) const { return _array[pos]; }
  volatile long   target_gen() const { return _target_gen; }
  volatile long * target_gen_addr()  { return &_target_gen; }

  void   update(uint age);
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for NGenerationArray...");
    return (unsigned int)0;
  } // placeholder
};

class ThreadLocalNGenMapping : public CHeapObj<mtGC>
{
 private:
  // The hash value is used to prevent the loss of 1-1 correspondence between
  // hash and index in the thread-local table. In practice, using this array
  // you know that at index I, every thread will have a counter for objects
  // allocated at allocation site whose hash is stored at _hashes[I].
  uint * _hashes;

 public:
  ThreadLocalNGenMapping() {
    _hashes = NEW_C_HEAP_ARRAY(uint, NG2C_MAX_ALLOC_SITE, mtGC);
    memset(_hashes, 0, (NG2C_MAX_ALLOC_SITE) * sizeof(uint));
}
  ThreadLocalNGenMapping(uint * hashes) : _hashes(hashes) { }
  ThreadLocalNGenMapping(const ThreadLocalNGenMapping& copy) : _hashes(copy.hashes()) { }

  uint *  hashes() const { return _hashes; }
  uint ** hashes_addr()  { return &_hashes;}
  uint    get_slot(uint hash)
  {
    uint idx = hash % NG2C_MAX_ALLOC_SITE;
    while (_hashes[idx % NG2C_MAX_ALLOC_SITE] && _hashes[idx] != hash) idx++;
    if (!_hashes[idx]) _hashes[idx] = hash;
    return idx;
  }
};

#endif // SHARE_VM_NG2C_NG2C_GLOBALS_HPP
