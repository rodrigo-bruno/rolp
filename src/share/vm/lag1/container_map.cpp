# include "lag1/container_map.hpp"

/**
 * Static members initialization
 */
// uint
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
