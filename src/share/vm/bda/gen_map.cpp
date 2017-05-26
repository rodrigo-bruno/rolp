# include "bda/gen_map.hpp"

/**
 * Static members initialization
 */
// uint
// GenMap::_next_gen_id = GenMap::gen_zero;
// GrowableArray<GenMap::GenEl*> *
// GenMap::_bda_klass_names = new (ResourceObj::C_HEAP, mtGC) GrowableArray<GenEl*>(0, true);

/**
 * Static functions definition
 */

void
GenMap::parse_from_string(const char * line, void (GenMap::* parse_line)(char*))
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
GenMap::parse_from_line(char * line)
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
    GenEl* el = new GenEl(str, _next_gen_id);
    _next_gen_id++; // << region_shift;
    _bda_klass_names->push(el);
  }
}

void
GenMap::initialize()
{
  _next_gen_id = GenMap::gen_start;
  _bda_klass_names = new (ResourceObj::C_HEAP, mtGC) GrowableArray<GenEl*>(0, true);
  // Parse the launch parameter
  parse_from_string(LAClasses, &GenMap::parse_from_line);
}

bool
GenMap::register_entry_or_null(Klass * k)
{
  const char * klass_name = k->external_name();
  int idx = bda_klass_names()->find((void*)klass_name, GenEl::equals_name);
  if (idx >= 0) {
    const GenEl * el  = bda_klass_names()->at(idx);
    const uint gen_id = el->gen_id();
    k->set_gen_id(gen_id);
    return true;
  } else {
    k->set_gen_id(gen_zero);
    return false;
  }
}

void
GenMap::print_klass_names(outputStream * ostream)
{
  for (int i = 0; i < bda_klass_names()->length(); i++) {
    const GenEl * el = bda_klass_names()->at(i);
    ostream->print_cr("[lap-trace-debug] klass name: %s gen_id " INT32_FORMAT, el->klass_name(), el->gen_id() );
  }
}
