#ifndef SHARE_VM_LAG1_LAG1OOPCLOSURES_INLINE_HPP
#define SHARE_VM_LAG1_LAG1OOPCLOSURES_INLINE_HPP

# include "lag1/lag1OopClosures.hpp"
# include "lag1/container_map.hpp"
# include "oops/markOop.inline.hpp"

template <class T>
inline void
LAG1ParMarkDSClosure::do_oop_work(T * p)
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
    if (obj->cas_claim_oop()) {
      // Create new alloc region
      GenAllocRegion * ct_alloc_region = _g1->new_container_gen();
      // Add the hashed address of the parent and the ct_alloc_region to the hashtable
      // TODO: Is this needed? Maybe not...
      _g1->ct_alloc_hashtable()->add_alloc_region(obj, ct_alloc_region);
      // TODO: Use the mark below to install on the hashtable or let the calculate hash
      // do its thing?
      
      // Calculate a 32bit offset to the ct_alloc_region from the end of the reserved part
      // of the Java heap
      uintptr_t offset = calculate_offset(ct_alloc_region);
      assert (mask_bits_are_true(right_n_bits(markOopDesc::lag1_offset_bits), offset), "sanity");
      
      // Now put the address of the alloc_region in the header.
      // No need to cas, the object should be owned by this thread
      obj->install_allocr(offset, _offset_base < (HeapWord*)offset ? true : false);
#ifdef LAG1_DEBUG_TRACING
      gclog_or_tty->print_cr("[lag1-debug-tracing] oop " INTPTR_FORMAT
                             " claimed and installed mark "INTPTR_FORMAT
                             " offset_base "INTPTR_FORMAT,
                             (intptr_t)obj, (intptr_t)obj->mark(), _par_scan_state->offset_base());
#endif

      // TODO:
      // assert(((void*)ct_alloc_region) ==  _offset_base + mark) "pointer to gc alloc region do not match mark");

      // Push the contents to a queue to be subject to the same treatment
      // TODO:
      // There are two ways of doing this:
      // a) Set the offset part of the mark (the mark) in the scanner and, while scanning,
      //    use the kept value to install on the scanned field.
      // b) Implement a new iterator adapted to receive two oops, the parent
      //    and the the field in question. Then the parent's offset part of the mark
      //    can be easily read. The new iterator can also carry the offset instead of the
      //    parent oop.
      // Here we are using option (a) but option (b) has a strong case, because it avoids
      // several memory accesses for every Java object over the scanner VM object.
      _ds_scanner.set_offset_mark((uint32_t)obj->allocr());
      obj->oop_iterate_backwards(&_ds_scanner);
    }
  }
}

template <class T>
inline void
LAG1ParMarkFollowerClosure::do_oop_work(T * p, uint32_t m)
{
  oop obj = oopDesc::load_decode_heap_oop(p);
  assert (_worker_id == _par_scan_state->queue_num(), "sanity");
  // install the offset mark in the oop p
  if(!obj->has_allocr() && obj->cas_install_allocr((uintptr_t)m)) {
#ifdef LAG1_DEBUG_TRACING
    gclog_or_tty->print_cr("[lag1-debug-tracing] children oop " INTPTR_FORMAT " is marked, mark is "INTPTR_FORMAT,
                           (intptr_t)obj, (intptr_t)obj->mark());
#endif
    // push its references to the stacks using a scanner
    // saving the mark first in the scanner object
    _ds_scanner.set_offset_mark(m);
    obj->oop_iterate_backwards(&_ds_scanner);
  }
}

template <class T>
inline void
LAG1ParScanDSClosure::do_oop_nv(T * p)
{
  T heap_oop = oopDesc::load_heap_oop(p);
  if (!oopDesc::is_null(heap_oop)) {
    oop obj = oopDesc::decode_heap_oop_not_null(heap_oop);
    // Here we prefetch the head of the object for write and read
    // since we will install the id of the container region (write)
    // and later we will get back at it to propagate to its followers
    Prefetch::write(obj->mark_addr(), 0);
    Prefetch::read(obj->mark_addr(), (HeapWordSize)*2);
    
    _par_scan_state->push_on_premark_queue(p, offset_mark());
  }
}

#endif // SHARE_VM_LAG1_LAG1OOPCLOSURES_INLINE_HPP
