#include "os.h"

void spinlock_lock()
{
  w_mstatus(r_mstatus() & ~MSTATUS_MIE);
}

void spinlock_unlock()
{
  w_mstatus(r_mstatus() | MSTATUS_MIE);
}