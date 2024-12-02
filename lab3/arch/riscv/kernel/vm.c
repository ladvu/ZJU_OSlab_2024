#include "vm.h"
#include "sched.h"
#include "stdio.h"
#include "test.h"

extern uint64_t text_start;
extern uint64_t rodata_start;
extern uint64_t data_start;
extern uint64_t _end;
extern uint64_t user_program_start;

uint64_t alloc_page() {
  // 从 &_end 开始分配一个页面，返回该页面的首地址
  // 注意：alloc_page始终返回物理地址
  // set the page to zero
  uint64_t ptr = PHYSICAL_ADDR((uint64_t)(&_end) + PAGE_SIZE * alloc_page_num++);
  for(int i = 0; i < PAGE_SIZE; i++) {
    ((char *)(ptr))[i] = 0;
  }
  return ptr;
}

int alloced_page_num(){
  // 返回已经分配的物理页面的数量
  return alloc_page_num;
}

void create_mapping(uint64_t *pgtbl, uint64_t va, uint64_t pa, uint64_t sz, int perm) {
    for(uint64_t i = 0; i < sz; i += PAGE_SIZE, va += PAGE_SIZE, pa += PAGE_SIZE) {
        // 1,2,3. 通过 va 得到三级页表项的索引
        uint64_t vpn1 = (va >> 12) & 0x1ff;  // 一级页表索引
        uint64_t vpn2 = (va >> 21) & 0x1ff;  // 二级页表索引
        uint64_t vpn3 = (va >> 30) & 0x1ff;  // 三级页表索引
        
        // 4. 如果三级页表项不存在，分配一个二级页表
        uint64_t *pgtb2;  // 二级页表基地址
        if ((pgtbl[vpn3] & 0x1) == 1) {
            // 5a. 如果存在，获取二级页表物理地址
            pgtb2 = PHYSICAL_ADDR((pgtbl[vpn3] >> 10) << 12);
        } else {
            // 5b. 如果不存在，分配新页面并设置页表项
            uint64_t PPN = alloc_page() >> 12;
            pgtbl[vpn3] = (pgtbl[vpn3] & 0xffc0000000000000) |  // 保留高位
                         (PPN << 10) |                           // 设置物理页号
                         0x1;                                    // 设置有效位
            pgtb2 = PHYSICAL_ADDR((pgtbl[vpn3] >> 10) << 12);
        }

        // 6. 如果二级页表项不存在，分配一个一级页表
        uint64_t *pgtb1;  // 一级页表基地址
        if ((pgtb2[vpn2] & 0x1) == 1) {
            // 7a. 如果存在，获取一级页表物理地址
            pgtb1 = PHYSICAL_ADDR((pgtb2[vpn2] >> 10) << 12);
        } else {
            // 7b. 如果不存在，分配新页面并设置页表项
            uint64_t PPN = alloc_page() >> 12;
            pgtb2[vpn2] = (pgtb2[vpn2] & 0xffc0000000000000) |  // 保留高位
                         (PPN << 10) |                           // 设置物理页号
                         0x1;                                    // 设置有效位
            pgtb1 = PHYSICAL_ADDR((pgtb2[vpn2] >> 10) << 12);
        }

        // 8. 设置一级页表项的内容
        pgtb1[vpn1] = (pgtb1[vpn1] & 0xffc0000000000000) |  // 保留高位
                     ((pa >> 12) << 10) |                    // 设置物理页号
                     perm;                                   // 设置权限位
    }
}

void paging_init() { 
  // 在 vm.c 中编写 paging_init 函数，该函数完成以下工作：
  // 1. 创建内核的虚拟地址空间，调用 create_mapping 函数将虚拟地址 0xffffffc000000000 开始的 16 MB 空间映射到起始物理地址为 0x80000000 的 16MB 空间，PTE_V | PTE_R | PTE_W | PTE_X 为映射的读写权限。
  // 2. 对内核起始地址 0x80000000 的16MB空间做等值映射（将虚拟地址 0x80000000 开始的 16 MB 空间映射到起始物理地址为 0x80000000 的 16MB 空间），PTE_V | PTE_R | PTE_W | PTE_X 为映射的读写权限。
  // 3. 修改对内核空间不同 section 所在页属性的设置，完成对不同section的保护，其中text段的权限为 r-x, rodata 段为 r--, 其他段为 rw-，注意上述两个映射都需要做保护。
  // 4. 将必要的硬件地址（如 0x10000000 为起始地址的 UART ）进行等值映射 ( 可以映射连续 1MB 大小 )，无偏移，PTE_V | PTE_R | PTE_W 为映射的读写权限

  // 注意：paging_init函数创建的页表只用于内核开启页表之后，进入第一个用户进程之前。进入第一个用户进程之后，就会使用进程页表，而不再使用 paging_init 创建的页表。

  uint64_t *pgtbl = alloc_page();
  // TODO: 请完成你的代码
  create_mapping( pgtbl, 0xffffffc000000000, 0x80000000, 0x1000000, PTE_V | PTE_R | PTE_W | PTE_X );
  create_mapping( pgtbl, 0x80000000, 0x80000000, 0x1000000, PTE_V | PTE_R | PTE_W | PTE_X );
  create_mapping( pgtbl, (uint64_t)(&text_start) + offset, (uint64_t)(&text_start), (uint64_t)(&rodata_start) - (uint64_t)(&text_start), PTE_V | PTE_R | PTE_X) ;
  create_mapping( pgtbl, (uint64_t)(&text_start), (uint64_t)(&text_start), (uint64_t)(&rodata_start) - (uint64_t)(&text_start), PTE_V | PTE_R | PTE_X) ;
  create_mapping( pgtbl, (uint64_t)(&rodata_start) + offset, (uint64_t)(&rodata_start), (uint64_t)(&data_start) - (uint64_t)(&rodata_start), PTE_V | PTE_R ) ;
  create_mapping( pgtbl, (uint64_t)(&rodata_start), (uint64_t)(&rodata_start), (uint64_t)(&data_start) - (uint64_t)(&rodata_start), PTE_V | PTE_R ) ;
  create_mapping( pgtbl, (uint64_t)(&data_start) + offset, (uint64_t)(&data_start), (uint64_t)(&user_program_start) - (uint64_t)(&data_start), PTE_V | PTE_R | PTE_W);
  create_mapping( pgtbl, (uint64_t)(&data_start), (uint64_t)(&data_start), (uint64_t)(&user_program_start) - (uint64_t)(&data_start), PTE_V | PTE_R | PTE_W);
  create_mapping( pgtbl, 0x10000000, 0x10000000, 0x100000, PTE_V | PTE_R | PTE_W);
  
}


