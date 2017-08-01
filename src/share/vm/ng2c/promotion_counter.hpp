#ifndef SHARE_VM_NG2C_PROMOTION_COUNTER_HPP
#define SHARE_VM_NG2C_PROMOTION_COUNTER_HPP

# include "ng2c/ng2c_globals.hpp"
# include "utilities/hashtable.hpp"

class PromotionCounters : public CHeapObj<mtGC>
{

 private:
  Hashtable<PromotionCounter*, mtGC> * _counters;
  PromotionCounter * add_counter(unsigned int hash);

 public:

  PromotionCounters(unsigned int size) :
    _counters(new Hashtable<PromotionCounter*, mtGC>(size, sizeof(HashtableEntry<PromotionCounter*, mtGC>))) { }

  PromotionCounter * get_counter_not_null(unsigned int hash);
  PromotionCounter * get_counter(unsigned int hash);
  Hashtable<PromotionCounter*, mtGC> * get_counters() { return _counters; }
  void print_on(outputStream * st, const char * tag = "promocounters");
};

#endif // SHARE_VM_NG2C_PROMOTION_COUNTER_HPP
