#include "sched.h"
#include "stdio.h"
#include "sched.h"
#include "mm.h"

int start_kernel() {
  puts("ZJU OSLAB 6 学号:3220102854 姓名:吴晨宇\n");
  puts("ZJU OSLAB 6 学号:3220106025 姓名:李宇怀\n");
  slub_init();
  task_init();

  // 设置第一次时钟中断
  asm volatile("ecall");
  
  call_first_process();
  dead_loop();
  return 0;
}
