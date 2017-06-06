#ifndef SHARE_VM_LAG1_LAG1OOPCLOSURES_INLINE_HPP
#define SHARE_VM_LAG1_LAG1OOPCLOSURES_INLINE_HPP

# include "lag1/lag1OopClosures.hpp"
# include "lag1/container_map.hpp"

class GenAllocRegion;

template <class T>
inline void LAG1ParMarkDSClosure::do_oop_work(T * p)
{
  T heap_oop = oopDesc::load_heap_oop(p);
  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);
    // Here we prefetch the head of the object for write and read
    // since we will install the id of the container region (write)
    // and later we will get back at it to propagate to its followers
    Prefetch::write(obj->mark_addr(), 0);
    Prefetch::read(obj->mark_addr(), (HeapWordSize)*2);

    // Claim the oop
#ifdef LAG1_DEBUG_TRACING
    if (!obj->mark()->lag1_claimed()) {
      gclog_or_tty->print_cr("[lag1-debug-tracing] oop " INTPTR_FORMAT
                             " is not yet claimed, mark is "INTPTR_FORMAT,
                             (intptr_t)obj, (intptr_t)obj->mark());
    }
#endif

    if (obj->cas_claim_oop()) {
#ifdef LAG1_DEBUG_TRACING
      gclog_or_tty->print_cr("[lag1-debug-tracing] oop " INTPTR_FORMAT
                             " is claimed, mark is "INTPTR_FORMAT,
                             (intptr_t)obj, (intptr_t)obj->mark());
#endif
      // Create new alloc region
      GenAllocRegion * ct_alloc_region = _g1->new_container_gen();
      // Add the hashed address of the parent and the ct_alloc_region to the hashtable
      _g1->ct_alloc_hashtable()->add_alloc_region(obj, ct_alloc_region);
      // TODO: Use the mark below to install on the hashtable or let the calculate hash
      // do its thing?
      
      // Calculate a 32bit offset to the ct_alloc_region from the end of the reserved part
      // of the Java heap
      uintptr_t mark = calculate_offset(ct_alloc_region);
      
      // Now put the address of the alloc_region in the header.
      // No need to cas, the object should be owned by this thread
      obj->install_allocr(mark);
#ifdef LAG1_DEBUG_TRACING
      gclog_or_tty->print_cr("[lag1-debug-tracing] oop " INTPTR_FORMAT
                             " is claimed, mark is "INTPTR_FORMAT,
                             (intptr_t)obj, (intptr_t)obj->mark());
#endif
      
      // Push the contents to a queue to be subject to the same treatment 
    }
  }
}

#endif // SHARE_VM_LAG1_LAG1OOPCLOSURES_INLINE_HPP
