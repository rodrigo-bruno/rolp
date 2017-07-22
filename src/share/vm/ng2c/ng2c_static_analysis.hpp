#ifndef SHARE_VM_NG2C_STATIC_ANALYSIS_HPP
#define SHARE_VM_NG2C_STATIC_ANALYSIS_HPP

# include "ng2c/ng2c_globals.hpp"
# include "utilities/hashtable.hpp"

class StaticAnalysis : public CHeapObj<mtInternal>
{

 private:
  // File that contains context information. The file is expected to follow 
  // this syntax:
  // MID:<mid>:<class name>.<method name>(<args>)
  // NID:<nid>:<class name>.<method name>(<args>):bci
  // Note: <mid> and <nid> are 16bit (represented as text) unique ids.
  const char* _input_file;

  // These maps take an hash and return a 16bit unique id.
  Hashtable<ContextIndex*, mtInternal> * _invoke2Context;
  Hashtable<ContextIndex*, mtInternal> * _alloc2Context;

  bool parse_from_file();
  uint add_index(Hashtable<ContextIndex*, mtInternal> * hashtable, char* method, char* bci, char* index);
  uint get_value(Hashtable<ContextIndex*, mtInternal> * hashtable, uint key);
  uint hash(Method * m, int bci);
  uint hash(Method * m);
  uint hash(char * m, char * bci);
  uint hash(char * m);;

 public:

  StaticAnalysis (const char* input_file);

  uint       get_invoke_context(Method * m);
  uint       get_alloc_context(Method * m, int bci);
};

#endif // SHARE_VM_NG2C_STATIC_ANALYSIS_HPP
