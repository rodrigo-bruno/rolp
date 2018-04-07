# include "ng2c/ng2c_static_analysis.hpp"
# include "memory/nogc.h"
# include "oops/method.hpp"
# include "classfile/altHashing.hpp"

# include <string.h>

uint
StaticAnalysis::hash(Method * m, int bci)
{
  char buf[1024];
	uint key = m->constMethod()->context();

  if (key == 0) {
    m->name_and_sig_as_C_string(buf, 1024);
    key = AltHashing::murmur3_32(37, (const jbyte*)buf, strlen(buf));
    m->constMethod()->set_context(key);

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-hashing-method] %s (len=%d) at %d; key="INTPTR_FORMAT, buf, strlen(buf), bci, key);
#endif

  }


  return key + bci;
}

uint
StaticAnalysis::hash(char * m, int bci)
{
  uint key = AltHashing::murmur3_32(37, (const jbyte*)m, strlen(m));

#ifdef DEBUG_NG2C_PROF_SANALYSIS
  gclog_or_tty->print_cr("[ng2c-sanalysis-hashing-string] %s (len=%d) at %d; key="INTPTR_FORMAT, m, strlen(m), bci, key);
#endif

  return key + bci;
}

uint
StaticAnalysis::add_index(Hashtable<ContextIndex*, mtGC> * hashtable, char * method, int bci, unsigned int index)
{
  uint key = hash(method, bci);
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
			char * sbci = strtok (NULL, ":");
      assert(sbci != NULL, "could not parse bci");
      bci = strtol(sbci, NULL, 10);
      key = add_index(_invoke2Context, method, bci, index);
      assert(get_value(_invoke2Context, key)->index() == index, "could not retrieve value from hashtable");
    }
    else if (!strncmp(type, "NID", sizeof("NID"))) {
      char * sbci = strtok (NULL, ":");
      assert(sbci != NULL, "could not parse bci");
      bci = strtol(sbci, NULL, 10);
      key = add_index(_alloc2Context, method, bci, index);
      assert(get_value(_alloc2Context, key)->index() == index, "could not retrieve value from hashtable");
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

ContextIndex*
StaticAnalysis::get_value(Hashtable<ContextIndex*, mtGC> * hashtable, uint key)
{
  HashtableEntry<ContextIndex*, mtGC> * entry = hashtable->bucket(hashtable->hash_to_index(key));

  if (entry == NULL) return NULL;

  while (entry->next() != NULL && entry->hash() != key) entry = entry->next();

  if (entry->hash() == key) {
    ContextIndex * ci = (ContextIndex*) entry->literal();
    return ci;
  }

  return NULL;
}



ContextIndex*
StaticAnalysis::get_invoke_context(Method * m, int bci)
{
  uint key = hash(m, bci);
	ContextIndex * ci = get_value(_invoke2Context, key);
  if (ci != NULL) {
    ci->set_method(m);
    ci->set_bci(bci);
    return ci;
  }
  return NULL;
}

ContextIndex*
StaticAnalysis::get_alloc_context(Method * m, int bci)
{
  uint key = hash(m, bci);
  ContextIndex * ci = get_value(_alloc2Context, key);
  if (ci != NULL) {
    ci->set_method(m);
    ci->set_bci(bci);
    return ci;
  }
  return NULL;
}

unsigned int
StaticAnalysis::get_invoke_index(Method * m, int bci)
{
  ContextIndex * ci = get_invoke_context(m, bci);
  if (ci == NULL) {
    return 0;
  } else {
    return ci->index();
  }
}

unsigned int
StaticAnalysis::get_alloc_index(Method * m, int bci)
{
  ContextIndex * ci = get_alloc_context(m, bci);
  if (ci == NULL) {
    return 0;
  } else {
    return ci->index();
  }
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

void
StaticAnalysis::more_context(bool need_more_context)
{
  for (int i = 0; i < _invoke2Context->table_size(); i++) {
    HashtableEntry<ContextIndex*, mtGC> * entry = _invoke2Context->bucket(i);
    for(; entry != NULL; entry = entry->next()) {
      ContextIndex * ci = entry->literal();
      uint key = entry->hash();
      // TODO - improve implementation!
//      if (ci->track_context() != need_more_context) {
//      if (true) {
//        ci->set_track_context(need_more_context);
//      }
      gclog_or_tty->print_cr("[ng2c-more_context] key="INTPTR_FORMAT" index="INTPTR_FORMAT " %s",
        key, ci->index(), ci->track_context() ? "ON" : "OFF");
    }
  }
}
