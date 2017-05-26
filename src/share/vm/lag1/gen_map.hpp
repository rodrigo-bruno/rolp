#ifndef SHARE_VM_BDA_GEN_MAP_HPP
#define SHARE_VM_BDA_GEN_MAP_HPP

# include "memory/resourceArea.hpp"

class ResourceMark;


/* The actual class that manages the mapping between gen ids and Klass objects. */
class GenMap : public CHeapObj<mtGC> {

 private:

  enum {
    gen_zero = 0,
    gen_start = 1,
    region_shift = gen_start
  };
  
  // Class that wraps the class names and the ids with they are promoted
  class GenEl : public CHeapObj<mtGC> {

   private:
    const char* _klass_name;
    const uint  _gen_id;
   public:
    GenEl(const char* klass_name, uint gen_id) :
      _klass_name(klass_name), _gen_id(gen_id) {}

    const char* klass_name() const { return _klass_name; }
    const uint  gen_id()     const { return _gen_id; }
    // routine to find the element with a specific char* value
    static bool equals_name(void* klass_name, GenEl* value) {
        return strcmp((char*)klass_name, value->klass_name()) == 0;
    }
  };
  //

  uint _next_gen_id;
  GrowableArray<GenEl*>* _bda_klass_names;
  // static BDARegion* _region_data;
  // int        region_data_sz;

  // parse the command line string BDAKlasses="..."
  void parse_from_string(const char* line, void (GenMap::* parse)(char*));
  void parse_from_line(char* line);

 public:
  

  GenMap() { }

  void initialize();

  // checks if a klass is bda type and returns the appropriate gen id
  // static uint is_bda_klass(Klass* k);
  // // Assembler version of is_bda_klass(Klass*)
  // static void is_bda_klass_asm(JavaThread * java_thread, Klass* k);

  GrowableArray<GenEl*> * bda_klass_names() { return _bda_klass_names; }
  // Retrieves the number of gens for locality aware policy objects
  int number_lap_gens() { return bda_klass_names()->length(); }
  // checks if a klass with "name" is a bda type
  bool is_bda_type(const char* name);
  // actually gets the gen id on where objects familiar to "name" live
  uint bda_type(Klass* k);
  // registers an entry on the klass ptr in the _gen_id field.
  bool register_entry_or_null(Klass * k);
  // adds an entry to the Hashtable of klass_ptr<->uint
  void add_entry(Klass* k); // TODO : change to register_klass_entry(..)
  // print klass names
  void print_klass_names(outputStream * ostream);

  // TODO FIXME : This is too see if any of these are needed.
  // adds an entry for the general object space
  // inline void add_other_entry(Klass* k);
  // // adds an entry for one of the bda spaces
  // inline void add_region_entry(Klass* k, BDARegion* r);
  // // an accessor for the region_map which is used by set_klass(k) to assign a bda space
  // static BDARegion* region_for_klass(Klass* k);
  // // an accessor for the _region_data array which saves the uint wrapper
  // static BDARegion* region_data() { return _region_data; }
  // // an accessor for the elements in the _region_data array
  // static BDARegion* region_elem(uint r) {
  //   int idx = 0;
  //   while(_region_data[idx].value() != r)
  //     ++idx;
  //   return &_region_data[idx];
  // }
  // // Fast accessor for no_region and the low_addr of the _region_data array
  // static BDARegion* no_region_ptr() { return &_region_data[0]; }
  // // Fast accessor for region_start (other's space)
  // static BDARegion* region_start_ptr() { return &_region_data[1]; }
  // // Fast accessor for the last element in the _region_data array
  // static BDARegion* last_region_ptr() { return &_region_data[_region_data_sz - 1]; }

};

#endif // SHARE_VM_BDA_GEN_MAP_HPP
