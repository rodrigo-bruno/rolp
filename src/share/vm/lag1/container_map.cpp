# include "lag1/container_map.hpp"
# include "classfile/altHashing.hpp"

/**
 * Static members initialization
 */
volatile int AllocRegionHashtable::_seed = 0;
// int
// ContainerMap::_next_ct_id = ContainerMap::ct_zero;
// GrowableArray<ContainerMap::ContainerEl*> *
// ContainerMap::_bda_klass_names = new (ResourceObj::C_HEAP, mtGC) GrowableArray<ContainerEl*>(0, true);

/**
 * Static functions definition
 */

void
ContainerMap::parse_from_string(const char * line, void (ContainerMap::* parse_line)(char*))
{
  char buffer[256];
  char delimiter = ',';
  const char* c = line;
  if (c != NULL) {
    int i = *c++;
    int pos = 0;
    while(i != '\0') {
      if(i == delimiter) {
        buffer[pos++] = '\0';
        (this->*parse_line)(buffer);
        pos = 0;
      } else {
        buffer[pos++] = i;
      }
      i = *c++;
    }
    // save the last one
    buffer[pos] = '\0';
    (this->*parse_line)(buffer);
  }
}

void
ContainerMap::parse_from_line(char * line)
{
  // Accept dots '.' and slashes '/' but not mixed.
  char delimiter;
  char buffer[64];
  char* str;
  bool delimiter_found = false;
  int i = 0;
  for(char* c = line; *c != '\0'; c++) {
    if(*c == '.' && !delimiter_found) {
      delimiter_found = true;
      delimiter = '.';
    } else if (*c == '/' && !delimiter_found) {
      delimiter_found = true;
      delimiter = '/';
    }
    if(*c == '/' || *c == '.')
      buffer[i++] = delimiter;
    else
      buffer[i++] = *c;
  }

  // This saves the hassle of dealing with empty class names
  if (i != 0) {
    buffer[i] = '\0';
    str = NEW_C_HEAP_ARRAY(char, i + 1, mtGC);
    strcpy(str, buffer);
    // construct the object and push while shifting the next_region value
    ContainerEl* el = new ContainerEl(str, _next_ct_id);
    _next_ct_id++; // << region_shift;
    _klass_names->push(el);
  }
}

void
ContainerMap::initialize()
{
  _next_ct_id = ContainerMap::ct_start;
  _klass_names = new (ResourceObj::C_HEAP, mtGC) GrowableArray<ContainerEl*>(0, true);
  // Parse the launch parameter
  parse_from_string(LAG1Classes, &ContainerMap::parse_from_line);
}

bool
ContainerMap::register_entry_or_null(Klass * k)
{
  const char * klass_name = k->external_name();
  int idx = klass_names()->find((void*)klass_name, ContainerEl::equals_name);
  if (idx >= 0) {
    const ContainerEl * el  = klass_names()->at(idx);
    const uint ct_id = el->ct_id();
    k->set_ct_id(ct_id);
    return true;
  } else {
    k->set_ct_id(ct_zero);
    return false;
  }
}

void
ContainerMap::print_klass_names(outputStream * ostream)
{
  for (int i = 0; i < klass_names()->length(); i++) {
    const ContainerEl * el = klass_names()->at(i);
    ostream->print_cr("[lap-trace-debug] klass name: %s ct_id " INT32_FORMAT, el->klass_name(), el->ct_id() );
  }
}

/* AllocRegionHashtable */

AllocRegionHashtable::AllocRegionHashtable(int table_size)
  : Hashtable<AllocRegionAddr*,mtGC>(table_size, sizeof(AllocRegionEntry)) { }

// TODO: Put the counter here?
uintptr_t
AllocRegionHashtable::add_alloc_region(oop p, G1AllocRegion * alloc_region)
{
  uint hash = AllocRegionHashtable::calculate_hash(p);
  AllocRegionAddr * alloc_region_addr = new AllocRegionAddr(alloc_region, hash);
  
  AllocRegionEntry * entry =
    (AllocRegionEntry*)Hashtable<AllocRegionAddr*,mtGC>::new_entry(hash, alloc_region_addr);

  assert (get_entry(hash) == NULL, "should exist none so far.");

  Hashtable<AllocRegionAddr*,mtGC>::add_entry(hash_to_index(hash), entry);

  return (uintptr_t)alloc_region;
}

AllocRegionEntry*
AllocRegionHashtable::get_entry(uint hash)
{
  AllocRegionEntry* entry = (AllocRegionEntry*)BasicHashtable<mtGC>::bucket(hash_to_index(hash));
  while (entry != NULL && entry->alloc_region()->hash() != hash) {
      entry = entry->next();
  }
  return entry;
}


unsigned int
AllocRegionHashtable::calculate_hash(oop p)
{
  Atomic::inc((volatile int*)&_seed);
  // sizeof(oop) should be a size of pointer
  unsigned int hash = (unsigned int)AltHashing::murmur3_32(_seed, (const jbyte *)p, sizeof(oop));
  return hash;
}
