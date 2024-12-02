#include "sched.h"
#include "stdio.h"
#include "sched.h"
#include "mm.h"
#include "virtio.h"

int start_kernel() {
  puts("ZJU OSLAB 7 学号:3220102854 姓名:吴晨宇\n");
  puts("ZJU OSLAB 7 学号:3220106025 姓名:李宇怀\n");
  
  slub_init();
  task_init();
  plic_init();
  
  virtio_disk_init();
  call_first_process();
  
  asm volatile("ecall");
  
  dead_loop();
  return 0;
}
