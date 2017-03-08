# include "ng2c/method_bci_hashtable.hpp"

MethodBciHashtable::MethodBciHashtable(int table_size)
  : Hashtable<gen_array_t, mtGC>(table_size, sizeof(MethodBciEntry)) {
}

unsigned long *
MethodBciHashtable::add_entry(Method * m, int bci)
{
  unsigned int hash = calculate_hash(m, bci);
  unsigned int rhash = mask_bits ((uintptr_t)hash, 0x1FFFFFF);
  unsigned long * array = NEW_C_HEAP_ARRAY(unsigned long, NG2C_GEN_ARRAY_SIZE, mtGC);
  MethodBciEntry * entry = (MethodBciEntry*)Hashtable<gen_array_t, mtGC>::new_entry(rhash, array);
  // return last element to save on generated method code for faster access
  return &array[NG2C_GEN_ARRAY_SIZE-1];
}

unsigned int
MethodBciHashtable::calculate_hash(Method * m, int bci)
{

}
