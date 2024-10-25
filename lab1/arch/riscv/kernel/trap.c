#ifndef PRINT_ONLY
#include "defs.h"
#include "clock.h"
#include "print.h"


void handler_s(uint64_t cause) {
  // TODO
  // interrupt
  if (cause >> 63 == 1) {
    // supervisor timer interrupt
    if (cause == (1ull << 63) + 5) {
      // 设置下一个时钟中断，打印当前的中断数目
      // TODO
      put_num(ticks);
      clock_set_next_event();
    }
  }
}
#endif