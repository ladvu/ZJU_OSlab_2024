#ifndef PRINT_ONLY
#include "riscv.h"
#include "clock.h"
#include "init.h"


void intr_enable(void) {
  // 设置 sstatus[sie] = 1, 打开 s 模式的中断开关
  set_csr(sstatus,2);
}

void intr_disable(void) {
  // 设置 sstatus[sie] = 0, 关闭 s 模式的中断开关
  clear_csr(sstatus,2);
}

void idt_init(void) {
  extern void trap_s(void);
  // 向 stvec 寄存器中写入中断处理后跳转函数的地址
  // TODO
  write_csr(stvec,trap_s);
}

void init(void) {
  idt_init();
  intr_enable();
  clock_init();
}
#endif