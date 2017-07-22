# include "ng2c/ng2c_static_analysis.hpp"
# include "memory/nogc.h"
# include "oops/method.hpp"
# include "classfile/altHashing.hpp"

# include <string.h>


uint
StaticAnalysis::hash(char * m, char * bci)
{
  int ibci = strtol(bci, NULL, 10);
  return AltHashing::murmur3_32(ibci, (const jchar*)m, strlen(m));
}

uint
StaticAnalysis::hash(char * m)
{
  // 41 is just a random prime number being used as a seed.
  return AltHashing::murmur3_32(41, (const jchar*)m, strlen(m));
}

uint
StaticAnalysis::hash(Method * m, int bci)
{
  return AltHashing::murmur3_32(bci, (const jbyte*)m->constMethod(), sizeof(ConstMethod));
}

uint
StaticAnalysis::hash(Method * m)
{
  // 37 is just a random prime number being used as a seed.
  return AltHashing::murmur3_32(37, (const jbyte*)m->constMethod(), sizeof(ConstMethod));
}

uint
StaticAnalysis::add_index(Hashtable<ContextIndex*, mtInternal> * hashtable, char * method, char * bci, char* index)
{
  uint key = bci == NULL ? hash(method) : hash(method, bci);
  int iindex = strtol(index, NULL, 10);
  ContextIndex * ci = new ContextIndex(iindex);
  HashtableEntry<ContextIndex*, mtInternal> * entry = hashtable->new_entry(key, ci);

  assert(entry != NULL, "static analysis could not add new entry to hashmap");

  hashtable->add_entry(hashtable->hash_to_index(key), entry);

  return key;
}

bool
StaticAnalysis::parse_from_file() {
  assert(_input_file != NULL, "Static analysis file not provided.");
  FILE* stream = fopen(_input_file, "rt");
  if (stream == NULL) {
    gclog_or_tty->print_cr("[ng2c-sanalysis] failed to open %s (errno=%d)", _input_file, errno);
    return false;
  }

  size_t bf_sz = 8*1024 + 1; // Just to make sure strcspn does not crash.
  uint key = 0;
  char line[bf_sz];

  while(fgets(line, bf_sz, stream)) {
    line[strcspn(line, "\n")] = 0;

    char* type = strtok (line, ":");
    char* index = strtok (NULL, ":");
    char* method = strtok (NULL, ":");
    char* bci = strtok (NULL, ":");

    if (type == NULL || method == NULL || index == NULL) {
      gclog_or_tty->print_cr("[ng2c-sanalysis] file = %s: unparsable line = %s", 
          NG2CStaticAnalysis, line);
      return false;
    }
    else if (!strncmp(type, "MID", sizeof("MID"))) {
      key = add_index(_invoke2Context, method, bci, index);
    } 
    else if (!strncmp(type, "NID", sizeof("NID"))) {
      key = add_index(_alloc2Context, method, bci, index);
    } 
    else {
      gclog_or_tty->print_cr("[ng2c-sanalysis] file = %s: unknown type = %s", 
          NG2CStaticAnalysis == NULL ? "null" : NG2CStaticAnalysis,
          type);
      return false;
    }

#ifdef DEBUG_NG2C_PROF_SANALYSIS
    gclog_or_tty->print_cr("[ng2c-sanalysis] target=%s method=%s bci=%s index=%s",
        type, method, bci, index);
#endif
  }

  fclose(stream);
  return true;
}

StaticAnalysis::StaticAnalysis(const char* input_file) : 
    _input_file(input_file), 
    _invoke2Context(new Hashtable<ContextIndex*, mtInternal>(NG2C_MAX_ALLOC_SITE, sizeof(ContextIndex))), 
    _alloc2Context(new Hashtable<ContextIndex*, mtInternal>(NG2C_MAX_ALLOC_SITE, sizeof(ContextIndex)))
{
#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis] parsing file=%s", NG2CStaticAnalysis);
#endif
  parse_from_file();
}

uint
StaticAnalysis::get_value(Hashtable<ContextIndex*, mtInternal> * hashtable, uint key)
{
  HashtableEntry<ContextIndex*, mtInternal> * entry = hashtable->bucket(hashtable->hash_to_index(key));

  if (entry == NULL) return 0;

  while (entry->next() != NULL && entry->hash() != key) entry = entry->next();

  if (entry->hash() == key) {
    ContextIndex * ci = (ContextIndex*) entry->literal();
    return ci->index();
  }

  return 0;
}

uint
StaticAnalysis::get_invoke_context(Method * m)
{
  uint key = hash(m);
  return get_value(_invoke2Context, key);
}

uint
StaticAnalysis::get_alloc_context(Method * m, int bci)
{
  uint key = hash(m, bci);
  return get_value(_alloc2Context, key);
}

