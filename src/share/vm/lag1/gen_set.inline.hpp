#ifndef SHARE_VM_LAG1_GEN_SET_INLINE_HPP
#define SHARE_VM_LAG1_GEN_SET_INLINE_HPP

# include "lag1/gen_set.hpp"

template <class E, MEMFLAGS F>
GenLinkedQueue<E, F>::GenLinkedQueue() :
  GenSetBase<F>::GenSetBase(),
  _head(NULL),
  _tail(NULL)
{
  _mutex = new Monitor(Mutex::nonleaf+3, "GenLinkedQueue lock");
}

template <class E, MEMFLAGS F>
inline void
GenLinkedQueue<E, F>::push(E el)
{
  E temp = NULL;
  do {
    temp = head();
  } while (Atomic::cmpxchg_ptr(el, &_head, temp) != temp);

  if (temp != NULL) temp->set_next(el);
  // If this is the first element
  if (temp == NULL && tail() == NULL) set_tail(el);
  GenSetBase<F>::inc_length_atomic();
}

// TODO: Return as (volatile E *)? Something to consider.
template <class E, MEMFLAGS F>
inline E
GenLinkedQueue<E, F>::pop()
{
  E temp;
  E next;
  do {
    temp = tail();
    if (temp == NULL) return temp;
    next = temp->next();
  } while (Atomic::cmpxchg_ptr(next, &_tail, temp) != temp);

  // The list is now empty
  if (tail() == NULL) set_head(NULL);
  GenSetBase<F>::dec_length_atomic();
  return temp;
}

// TODO: Return the copy as (volatile E)? Something to consider.
template <class E, MEMFLAGS F>
inline E
GenLinkedQueue<E, F>::at(int pos)
{
  GenSetIterator<E, F> it = iterator();
  for (int i = 0; i < pos; ++it);
  return *it;
}

#endif // SHARE_VM_LAG1_GEN_SET_INLINE_HPP
