#ifndef SHARE_VM_NG2C_METHOD_BCI_HASHTABLE_HPP
#define SHARE_VM_NG2C_METHOD_BCI_HASHTABLE_HPP

# include "ng2c/ng2c_globals.hpp"
# include "utilities/hashtable.hpp"

class Method;

class MethodBciEntry : public HashtableEntry<NGenerationArray*, mtGC>
{
 public:
  MethodBciEntry * next() const {
    return (MethodBciEntry*)HashtableEntry<NGenerationArray*, mtGC>::next();
  }

  MethodBciEntry ** next_addr() {
    return (MethodBciEntry**)HashtableEntry<NGenerationArray*, mtGC>::next_addr();
  }
};

class MethodBciHashtable : public Hashtable<NGenerationArray*, mtGC>
{
 // Note: this is necessary because some methods used in this friend class
 // require access to buckets. Better solution?
 friend class NG2C_MergeAllocCounters;
 public:

  MethodBciHashtable (int table_size);

  unsigned int       add_entry (Method * m, int bci);
  unsigned int       add_entry (uint hash);
  NGenerationArray * get_entry(uint hash);
  NGenerationArray * get_entry_not_null(uint hash);
  ngen_t *           get_alloc_slot(uint hash);
  volatile long    * get_target_gen(uint hash);
  static unsigned int calculate_hash(Method * m, int bci);
  void               print_on(outputStream * st, const char * tag = "global hashtable");
};

#endif // SHARE_VM_NG2C_METHOD_BCI_HASHTABLE_HPP
