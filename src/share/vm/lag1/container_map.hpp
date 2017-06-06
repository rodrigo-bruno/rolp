#ifndef SHARE_VM_LAG1_CONTAINER_MAP_HPP
#define SHARE_VM_LAG1_CONTAINER_MAP_HPP

# include "memory/resourceArea.hpp"
# include "utilities/hashtable.inline.hpp"

class ResourceMark;
class G1AllocRegion;

/* The actual class that manages the mapping between ct ids and Klass objects. */
class ContainerMap : public CHeapObj<mtGC>
{

 private:

  enum {
    ct_zero = 0,
    ct_start = 1,
    region_shift = ct_start
  };
  
  // Class that wraps the class names and the ids with they are promoted
  class ContainerEl : public CHeapObj<mtGC> {

   private:
    const char* _klass_name;
    const uint  _ct_id;
   public:
    ContainerEl(const char* klass_name, uint ct_id) :
      _klass_name(klass_name), _ct_id(ct_id) {}

    const char* klass_name() const { return _klass_name; }
    const uint  ct_id()     const { return _ct_id; }
    // routine to find the element with a specific char* value
    static bool equals_name(void* klass_name, ContainerEl* value) {
        return strcmp((char*)klass_name, value->klass_name()) == 0;
    }
  };
  //

  uint _next_ct_id;
  GrowableArray<ContainerEl*>* _klass_names;

  // parse the command line string LAG1Classes="..."
  void parse_from_string(const char* line, void (ContainerMap::* parse)(char*));
  void parse_from_line(char* line);

 public:

  ContainerMap() { }

  void initialize();

  GrowableArray<ContainerEl*> * klass_names() { return _klass_names; }
  // Retrieves the number of klasses for locality aware policy objects
  int number_klasses() { return klass_names()->length(); }
  // registers an entry on the klass ptr in the _ct_id field.
  bool register_entry_or_null(Klass * k);
  // print klass names
  void print_klass_names(outputStream * ostream);

};


/* AllocRegionAddr is a class that saves the address of the alloc_region for a
 * given container generation */
class AllocRegionAddr : public CHeapObj<mtGC>
{
  friend class AllocRegionHashtable;
  
  G1AllocRegion * _alloc_region;
  unsigned int    _hash;

 public:
  AllocRegionAddr(G1AllocRegion * alloc_region, uint hash) :
    _alloc_region(alloc_region), _hash(hash) { }

  G1AllocRegion * alloc_region() const { return _alloc_region; }
  
  // placeholders
  unsigned int new_hash (int seed) {
    assert(false, "new_hash called for AllocRegionAddr...");
    return (unsigned int)0;
  }
  
 protected:
  unsigned int    hash()         const { return _hash; }
};


class AllocRegionEntry : public HashtableEntry<AllocRegionAddr*,mtGC>
{
 public:
  // Literal
  AllocRegionAddr *  alloc_region()      { return (AllocRegionAddr*)literal(); }
  AllocRegionAddr ** alloc_region_addr() { return (AllocRegionAddr**)literal_addr(); }
  
  AllocRegionEntry * next() const {
    return (AllocRegionEntry*)HashtableEntry<AllocRegionAddr*,mtGC>::next();
  }
  AllocRegionEntry * next_addr() {
    return (AllocRegionEntry*)HashtableEntry<AllocRegionAddr*,mtGC>::next_addr();
  }
};


class AllocRegionHashtable : public Hashtable<AllocRegionAddr*,mtGC>
{

 private:

  /* Calculates an unique hash for the oop using as seed an atomically incremented counter,
   * which by default is the _seed value */
  static volatile int _seed;
  static unsigned int calculate_hash(oop p);
  
 public:
  
  AllocRegionHashtable(int table_size);

  /* Adds a new entry in the hashtable, using the less-significant 32 bits of the oop
   * as the hash. */
  uintptr_t add_alloc_region(oop p, G1AllocRegion * alloc_region);

  /* Gets an entry */
  AllocRegionEntry * get_entry(uint hash);

  /* Clears a falsely created entry due to eager pre-marking */
  void clear_alloc_region(G1AllocRegion * alloc_region);
};


#endif // SHARE_VM_LAG1_CONTAINER_MAP_HPP
