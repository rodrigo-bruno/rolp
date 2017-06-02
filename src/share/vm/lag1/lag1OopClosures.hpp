#ifndef SHARE_VM_LAG1_LAG1OOPCLOSURES_HPP
#define SHARE_VM_LAG1_LAG1OOPCLOSURES_HPP

class G1ParClosureSuper;
class G1CollectedHeap;
class G1ParScanThreadState;

class LAG1ParMarkDSClosure : public G1ParClosureSuper
{
  template <class T> void do_oop_work(T * p);

 protected:
  // Marks an object with a special bit field in the header,
  // corresponding to the target region for its whole data-structure graph
  void mark_lag1_object(oop obj);

 public:
  LAG1ParMarkDSClosure(G1CollectedHeap * g1, G1ParScanThreadState * par_scan_state) :
    G1ParClosureSuper(g1, par_scan_state) { }
  
  template <class T> void do_oop_nv(T * p) { do_oop_work(p); }
  virtual void do_oop(oop * p)       { do_oop_nv(p); }
  virtual void do_oop(narrowOop * p) { do_oop_nv(p); }
};

#endif // SHARE_VM_LAG1_LAG1OOPCLOSURES_HPP
