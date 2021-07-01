#include "os.h"

void lock_init(lock_t *lock)
{
  lock->locked = 0;
}

void lock_acquire(lock_t *lock)
{
  for (;;)
  {
    if (!atomic_swap(lock))
    {
      break;
    }
  }
}

void lock_free(lock_t *lock)
{
  lock->locked = 0;
}

void basic_lock()
{
  w_mstatus(r_mstatus() & ~MSTATUS_MIE);
}

void basic_unlock()
{
  w_mstatus(r_mstatus() | MSTATUS_MIE);
}
