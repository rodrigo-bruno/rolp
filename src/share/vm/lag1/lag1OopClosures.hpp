#ifndef SHARE_VM_LAG1_LAG1OOPCLOSURES_HPP
#define SHARE_VM_LAG1_LAG1OOPCLOSURES_HPP

class G1ParClosureSuper;
class G1CollectedHeap;
class G1ParScanThreadState;

class LAG1ParMarkDSClosure : public G1ParClosureSuper
{
  // For offset calculation
  HeapWord * _g1h_reserved_end;
  
  template <class T> void do_oop_work(T * p);

  /* Encodes the offset of the ptr to the alloc region using the reserved_end of the heap
   * as base */
  uintptr_t calculate_offset(void * region)
    { return pointer_delta(region, _g1h_reserved_end, 1); }

 protected:
  // Marks an object with a special bit field in the header,
  // corresponding to the target region for its whole data-structure graph
  void mark_lag1_object(oop obj);

 public:
  LAG1ParMarkDSClosure(G1CollectedHeap * g1, G1ParScanThreadState * par_scan_state) :
    G1ParClosureSuper(g1, par_scan_state) { _g1h_reserved_end = g1->g1_reserved().end(); }
  
  template <class T> void do_oop_nv(T * p) { do_oop_work(p); }
  virtual void do_oop(oop * p)       { do_oop_nv(p); }
  virtual void do_oop(narrowOop * p) { do_oop_nv(p); }
};

#endif // SHARE_VM_LAG1_LAG1OOPCLOSURES_HPP
