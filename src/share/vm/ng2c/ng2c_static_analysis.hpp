#ifndef SHARE_VM_NG2C_STATIC_ANALYSIS_HPP
#define SHARE_VM_NG2C_STATIC_ANALYSIS_HPP

# include "ng2c/ng2c_globals.hpp"
# include "utilities/hashtable.hpp"

class StaticAnalysis : public CHeapObj<mtInternal>
{

 private:
  const char* _input_file;
  Hashtable<ContextIndex*, mtInternal> * _invoke2index;
  Hashtable<ContextIndex*, mtInternal> * _alloc2index;

  bool parse_from_file();
  uint add_index(Hashtable<ContextIndex*, mtInternal> * hashtable, char* method, char* bci, char* index);
  uint get_value(Hashtable<ContextIndex*, mtInternal> * hashtable, uint key);
  uint hash(Method * m, int bci);
  uint hash(char * m, char * bci);

 public:

  StaticAnalysis (const char* input_file);

  // This method returns the index of the bit that should be set/unset by the
  // method call at 'm' and at 'bci'.
  uint       get_invoke_index (Method * m, int bci);

  // This method returns the intex of the first bit of the slot/window that
  // characterizes the context for the alloc site at 'm' and at 'bci'.
  uint       get_alloc_slot (Method * m, int bci);
};

#endif // SHARE_VM_NG2C_STATIC_ANALYSIS_HPP
