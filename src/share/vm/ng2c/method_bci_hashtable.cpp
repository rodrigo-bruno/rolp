# include "ng2c/method_bci_hashtable.hpp"
# include "classfile/altHashing.hpp"
# include "memory/nogc.h"

MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<NGenerationArray*, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

unsigned int
MethodBciHashtable::add_entry(Method * m, int bci)
{
  unsigned int hash = calculate_hash(m, bci);
  unsigned int rhash = mask_bits ((uintptr_t)hash, 0x1FFFFFF);
  NGenerationArray * array = new NGenerationArray();
  Hashtable<NGenerationArray*, mtGC>::new_entry(rhash, array);
  // TODO - memset to zero.
  return rhash;
}

unsigned int*
MethodBciHashtable::get_target_gen(unsigned int rhash)
{
    // TOOD - implement.
    return 0;
}

void
MethodBciHashtable::update_target_gen(unsigned int rhash, NGenerationArray* array)
{
    // TODO - implement.
}

unsigned int
MethodBciHashtable::calculate_hash(Method * m, int bci)
{
  unsigned int hash = (unsigned int)AltHashing::murmur3_32(bci, (const jbyte*)m, sizeof(Method));
#ifdef DEBUG_NG2C_PROF
  gclog_or_tty->print_cr("[ng2c-prof] object hash = " INTPTR_FORMAT, (intptr_t)hash);
#endif
  return hash;
}
