#include "unistd.h"
#include "task.h"
#include "os.h"
int fork()
{
  // -1 ： 發生錯誤
  // 0 ： 代表為子程序
  // 大於 0 ： 代表為父程序, 其回傳值為子程序的 ProcessID
  int parent = get_current_task();
  return task_fork(parent);
}
int getpid()
{
  return get_current_task();
}