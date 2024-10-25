#include "defs.h"
#include "print.h"

extern struct sbiret sbi_call(uint64_t ext, uint64_t fid, uint64_t arg0,
                              uint64_t arg1, uint64_t arg2, uint64_t arg3,
                              uint64_t arg4, uint64_t arg5);

int puts(char *str) {
  char * p = str;
  while (*p) {
    sbi_call(1,0,*p,0,0,0,0,0);
    p++;
  }
  return 1;
}

int put_num(uint64_t n) {
  if (!n) {
    sbi_call(1,0,48,0,0,0,0,0);
  }
  int cnt = 1;
  uint64_t temp = n;
  while (temp / 10) {
    cnt *= 10;
    temp /= 10;
  }
  while (n) {
    sbi_call(1,0,48 + n / cnt,0,0,0,0,0);
    n %= cnt;
    cnt /= 10;
  }
  return 1;
}