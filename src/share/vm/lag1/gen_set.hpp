#ifndef SHARE_VM_LAG1_GEN_SET_HPP
#define SHARE_VM_LAG1_GEN_SET_HPP

# include "memory/allocation.hpp"

// forward declarations
template <class E, MEMFLAGS F> class GenSetIterator;


template <MEMFLAGS F>
class GenSetBase : public CHeapObj<F>
{
  // The number of elements in the set.
  // TODO: This is int to support the rest of the code (for loops, etc),
  // but should be made uint since it makes no sense having less-than 0 values
  int _length;

 protected:
  void incr_length() { _length++; }
  void decr_length() { _length--; }
  void inc_length_atomic() { Atomic::inc((volatile int*)&_length); }
  void dec_length_atomic() { Atomic::dec((volatile int*)&_length); }

  int length() const { return _length; }
  bool is_empty() const { return length() == 0; }
  
 public:
  GenSetBase() : _length(0) { }
  
};

/* GenLinkedQueue functions much as a FIFO queue; push() inserts on the head and pop() removes
 * from the tail.
 * TODO: Maybe create first a LinkedList and only then specialize as a queue?
 */
template <class E, MEMFLAGS F>
class GenLinkedQueue : public GenSetBase<F>
{
  Monitor *  _mutex;
  
  volatile E _head;
  volatile E _tail;

 public:

  // Never takes any arguments
  GenLinkedQueue();

  void set_head(E head) { _head = head; }
  void set_tail(E tail) { _tail = tail; }

  // TODO: Return the copy as (volatile E)? Something to consider.
  E head() const { return (E)_head; }
  E tail() const { return (E)_tail; }

  // Inherited declarations
  int length() const { return GenSetBase<F>::length(); }
  bool is_empty() const { return GenSetBase<F>::is_empty(); }

  inline void push(E el);
  inline E  pop();
  inline E  at(int pos);

  // Get a STL-style iterator
  GenSetIterator<E, F> iterator() const { return GenSetIterator<E, F>(this); }
  
};

template <class E, MEMFLAGS F>
class GenSetIterator : public StackObj
{
  friend class GenLinkedQueue<E, F>;
  
 private:
  const GenLinkedQueue<E, F> * _set; 
  E                            _current;

  GenSetIterator(const GenLinkedQueue<E, F> * set) :
    _set(set)
    {
      assert (set->head() != NULL && set->tail() != NULL, "queue is malformed or empty");
      _current = _set->tail();
    }

 public:
  GenSetIterator<E, F>& operator++() { _current = _current->next(); return *this; }
  E                     operator* () { return _current; }
  
};

#endif // SHARE_VM_LAG1_GEN_SET_HPP
