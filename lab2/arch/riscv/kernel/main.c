#include "init.h"
#include "stdio.h"
#include "test.h"
#include "sched.h"


int main() {
  puts("ZJU OS LAB  2    Student_ID:3220102854, 3220106025\n");
  #ifdef SJF
    printf("initialize SJF test case....\n");
  #endif
  #ifdef PRIORITY
    printf("initialize PRIORITY test case....\n");
  #endif
  init();
  init_test_case();
  call_first_process();
  return 0;
}
