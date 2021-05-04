#include "unistd.h"
#include "task.h"
#include "os.h"
int fork()
{
  int parent = get_current_task();
  return task_fork(parent);
}
void wait()
{
  task_create(&task_killer);
}
int getpid()
{
  return get_current_task();
}