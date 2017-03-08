#ifndef SHARE_VM_NG2C_METHOD_BCI_HASHTABLE_HPP
#define SHARE_VM_NG2C_METHOD_BCI_HASHTABLE_HPP

# include "ng2c/ng2c_globals.hpp"
# include "oops/method.hpp"
# include "utilities/hashtable.hpp"

class MethodBciEntry : public HashtableEntry<gen_array_t, mtGC>
{
 public:
  MethodBciEntry * next() const {
    return (MethodBciEntry*)HashtableEntry<gen_array_t, mtGC>::next();
  }

  MethodBciEntry ** next_addr() {
    return (MethodBciEntry**)HashtableEntry<gen_array_t, mtGC>::next_addr();
  }
};

class MethodBciHashtable : public Hashtable<gen_array_t, mtGC>
{
 public:

  MethodBciHashtable (int table_size);

  unsigned long * add_entry (Method * m, int bci);
  unsigned int calculate_hash(Method * m, int bci);
};

#endif // SHARE_VM_NG2C_METHOD_BCI_HASHTABLE_HPP
