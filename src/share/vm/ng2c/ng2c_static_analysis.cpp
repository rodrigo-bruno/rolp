# include "ng2c/ng2c_static_analysis.hpp"
# include "memory/nogc.h"
# include "oops/method.hpp"
# include "classfile/altHashing.hpp"

# include <string.h>


uint
StaticAnalysis::hash(char * m)
{
  // 37 is just a random prime number being used as a seed.
  uint key = AltHashing::murmur3_32(37, (const jbyte*)m, strlen(m));

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-hashing] %s (len=%d); key="INTPTR_FORMAT, m, strlen(m), key);
#endif

  return key;
}

uint
StaticAnalysis::hash(Method * m, int bci)
{
  char buf[1024];
  m->name_and_sig_as_C_string(buf, 1024);
  uint key = AltHashing::murmur3_32(bci, (const jbyte*)buf, strlen(buf));

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-hashing] %s (len=%d) at %d; key="INTPTR_FORMAT, buf, strlen(buf), bci, key);
#endif

  return key;
}

uint
StaticAnalysis::hash(char * m, int bci)
{
  uint key = AltHashing::murmur3_32(bci, (const jbyte*)m, strlen(m));

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-hashing] %s (len=%d) at %d; key="INTPTR_FORMAT, m, strlen(m), bci, key);
#endif

  return key;
}

uint
StaticAnalysis::hash(Method * m)
{
  char buf[1024];
  m->name_and_sig_as_C_string(buf, 1024);
  // 37 is just a random prime number being used as a seed.
  uint key = AltHashing::murmur3_32(37, (const jbyte*)buf, strlen(buf));

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-hashing] %s (len=%d); key="INTPTR_FORMAT, buf, strlen(buf), key);
#endif

  return key;
}

uint
StaticAnalysis::add_index(Hashtable<ContextIndex*, mtGC> * hashtable, char * method, int bci, unsigned int index)
{
  uint key = bci < 0 ? hash(method) : hash(method, bci);
  ContextIndex * ci = new ContextIndex(index);
  HashtableEntry<ContextIndex*, mtGC> * entry = hashtable->new_entry(key, ci);
  int bucket = hashtable->hash_to_index(key);

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-add] entry="INTPTR_FORMAT" key="INTPTR_FORMAT" bucket=%d index=%d %s:%d", entry, key, bucket, index, method, bci);
#endif

  assert(entry != NULL, "static analysis could not add new entry to hashmap");

  hashtable->add_entry(bucket, entry);

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
  char line[bf_sz];

  while(fgets(line, bf_sz, stream)) {
    line[strcspn(line, "\n")] = 0;

    char* type = strtok (line, ":");
    // TODO - check for strol errors?
    unsigned int index = (unsigned short) strtol(strtok (NULL, ":"), NULL, 16);
    char* method = strtok (NULL, ":");
    uint key = 0;
    int bci = -1;

    if (type == NULL || method == NULL) {
      gclog_or_tty->print_cr("[ng2c-sanalysis] file = %s: unparsable line = %s",
          NG2CStaticAnalysis, line);
      return false;
    }
    else if (!strncmp(type, "MID", sizeof("MID"))) {
      key = add_index(_invoke2Context, method, bci, index);
      assert(get_value(_invoke2Context, key) == index, "could not retrieve value from hashtable");
    }
    else if (!strncmp(type, "NID", sizeof("NID"))) {
      char * sbci = strtok (NULL, ":");
      assert(sbci != NULL, "could not parse bci");
      bci = strtol(sbci, NULL, 10);
      key = add_index(_alloc2Context, method, bci, index);
      assert(get_value(_alloc2Context, key) == index, "could not retrieve value from hashtable");
    }
    else {
      gclog_or_tty->print_cr("[ng2c-sanalysis] file = %s: unknown type = %s",
          NG2CStaticAnalysis == NULL ? "null" : NG2CStaticAnalysis,
          type);
      return false;
    }

#ifdef DEBUG_NG2C_PROF_SANALYSIS
    gclog_or_tty->print_cr("[ng2c-sanalysis] target=%s method=%s bci=%d id="INTPTR_FORMAT" key="INTPTR_FORMAT,
        type, method, bci, index, key);
#endif
  }

  fclose(stream);

#ifdef PRINT_NG2C_PROF_SANALYSIS
  print_on(gclog_or_tty, _alloc2Context,  "alloc2Context");
  print_on(gclog_or_tty, _invoke2Context, "invoke2Context");
#endif

  return true;
}

StaticAnalysis::StaticAnalysis(const char* input_file) :
    _input_file(input_file),
    _invoke2Context(new Hashtable<ContextIndex*, mtGC>(NG2C_MAX_ALLOC_SITE, sizeof(HashtableEntry<ContextIndex*, mtGC>))),
    _alloc2Context(new Hashtable<ContextIndex*, mtGC>(NG2C_MAX_ALLOC_SITE, sizeof(HashtableEntry<ContextIndex*, mtGC>)))
{
#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis] parsing file=%s", NG2CStaticAnalysis);
#endif
  if (input_file != NULL) parse_from_file();
}

uint
StaticAnalysis::get_value(Hashtable<ContextIndex*, mtGC> * hashtable, uint key)
{
  HashtableEntry<ContextIndex*, mtGC> * entry = hashtable->bucket(hashtable->hash_to_index(key));

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

void
StaticAnalysis::print_on(outputStream * st, Hashtable<ContextIndex*, mtGC> * hashtable, const char * tag)
{
  for (int i = 0; i < hashtable->table_size(); i++) {
    HashtableEntry<ContextIndex*, mtGC> * entry = hashtable->bucket(i);
    for(; entry != NULL; entry = entry->next()) {
      ContextIndex * ci = entry->literal();
      uint key = entry->hash();
      st->print_cr("[ng2c-%s] key="INTPTR_FORMAT" index="INTPTR_FORMAT, tag, key, ci->index());
    }
  }
}

