# include "ng2c/method_bci_hashtable.hpp"
# include "classfile/altHashing.hpp"
# include "memory/nogc.h"

MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<NGenerationArray*, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

ngen_t
MethodBciHashtable::add_entry(Method * m, int bci)
{
  unsigned int hash = calculate_hash(m, bci);
  unsigned int rhash = mask_bits ((uintptr_t)hash, 0x1FFFFFF);
  NGenerationArray * array = new NGenerationArray();
  MethodBciEntry * entry =
    (MethodBciEntry*)Hashtable<NGenerationArray*, mtGC>::new_entry(rhash, array);
  // return last element to save on generated method code for faster access
  return array->at(NG2C_GEN_ARRAY_SIZE-1);
}

unsigned int
MethodBciHashtable::calculate_hash(Method * m, int bci)
{
  unsigned int hash = (unsigned int)AltHashing::murmur3_32(bci, (const jbyte*)m, sizeof(Method));
#if DEBUG_NG2C_PROF
  gclog_or_tty->print_cr("[ng2c-prof] object hash = " INTPTR_FORMAT, (intptr_t)hash);
#endif
  return hash;
}
