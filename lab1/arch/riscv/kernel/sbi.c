#include "defs.h"

struct sbiret sbi_call(uint64_t ext, uint64_t fid, uint64_t arg0, uint64_t arg1,
                       uint64_t arg2, uint64_t arg3, uint64_t arg4,
                       uint64_t arg5) {
  struct sbiret ret;
#ifndef TEST
// TODO:
  __asm__ volatile(
    "mv a0 %[arg0]\n"
    "mv a1 %[arg1]\n"
    "mv a2 %[arg2]\n"
    "mv a3 %[arg3]\n"
    "mv a4 %[arg4]\n"
    "mv a5 %[arg5]\n"
    "mv a6 %[fid]\n"
    "mv a7 %[ext]\n"
    "ecall\n"
    "mv %[error] a0\n"
    "mv %[value] a1"
    : [error] "=r" (ret.error), [value] "=r" (ret.value)
    : [arg0] "r" (arg0), [arg1] "r" (arg1), [arg2] "r" (arg2), [arg3] "r" (arg3), [arg4] "r" (arg4), [arg5] "r" (arg5), [fid] "r" (fid),[ext] "r" (ext)
    : "memory", "a0", "a1","a2","a3","a4","a5","a6","a7"
  );
#endif
  return ret;
}