#ifndef SHARE_VM_LAG1_CONTAINER_MAP_HPP
#define SHARE_VM_LAG1_CONTAINER_MAP_HPP

# include "memory/resourceArea.hpp"

class ResourceMark;


/* The actual class that manages the mapping between ct ids and Klass objects. */
class ContainerMap : public CHeapObj<mtGC> {

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

#endif // SHARE_VM_LAG1_CONTAINER_MAP_HPP
