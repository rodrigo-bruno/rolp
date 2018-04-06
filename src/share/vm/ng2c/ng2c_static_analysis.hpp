#ifndef SHARE_VM_NG2C_STATIC_ANALYSIS_HPP
#define SHARE_VM_NG2C_STATIC_ANALYSIS_HPP

# include "ng2c/ng2c_globals.hpp"
# include "utilities/hashtable.hpp"

class StaticAnalysis : public CHeapObj<mtGC>
{

 private:
  // File that contains context information. The file is expected to follow
  // this syntax:
  // MID:<mid>:<class name>.<method name>(<args>)
  // NID:<nid>:<class name>.<method name>(<args>):bci
  // Note: <mid> and <nid> are 16bit (represented as text) unique ids.
  const char* _input_file;

  // These maps take an hash and return a 16bit unique id.
  Hashtable<ContextIndex*, mtGC> * _invoke2Context;
  Hashtable<ContextIndex*, mtGC> * _alloc2Context;

  bool parse_from_file();
  unsigned int add_index(Hashtable<ContextIndex*, mtGC> * hashtable, char* method, int bci, unsigned int index);
  ContextIndex *get_value(Hashtable<ContextIndex*, mtGC> * hashtable, unsigned int key);
  void print_on(outputStream * st, Hashtable<ContextIndex*, mtGC> * hashtable, const char * tag = "sanalysis");
  uint hash(Method * m, int bci);
  uint hash(char * m, int bci);

 public:

  StaticAnalysis (const char* input_file);

  ContextIndex* get_invoke_context(Method * m, int bci);
  ContextIndex* get_alloc_context(Method * m, int bci);
  unsigned int  get_invoke_index(Method * m, int bci);
  unsigned int  get_alloc_index(Method * m, int bci);
  void          more_context(bool need_more_context);
};

#endif // SHARE_VM_NG2C_STATIC_ANALYSIS_HPP
