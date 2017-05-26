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

  // parse the command line string BDAKlasses="..."
  void parse_from_string(const char* line, void (GenMap::* parse)(char*));
  void parse_from_line(char* line);

 public:

  GenMap() { }

  void initialize();

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

};

#endif // SHARE_VM_BDA_GEN_MAP_HPP
