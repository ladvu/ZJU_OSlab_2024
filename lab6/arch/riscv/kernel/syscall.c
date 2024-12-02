#include "syscall.h"
#include "list.h"
#include "riscv.h"
#include "sched.h"
#include "task_manager.h"
#include "stdio.h"
#include "defs.h"
#include "slub.h"
#include "mm.h"
#include "vm.h"

extern uint64_t text_start;
extern uint64_t rodata_start;
extern uint64_t data_start;
extern uint64_t user_program_start;
extern void trap_s_bottom(void);

int strcmp(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (*a < *b)
            return -1;
        if (*a > *b)
            return 1;
        a++;
        b++;
    }
    if (*a && !*b)
        return 1;
    if (*b && !*a)
        return -1;
    return 0;
}

uint64_t get_program_address(const char *name)
{
    uint64_t offset = 0;
    if (strcmp(name, "hello") == 0)
        offset = PAGE_SIZE;
    else if (strcmp(name, "malloc") == 0)
        offset = PAGE_SIZE * 2;
    else if (strcmp(name, "print") == 0)
        offset = PAGE_SIZE * 3;
    else if (strcmp(name, "guess") == 0)
        offset = PAGE_SIZE * 4;
    else
    {
        printf("Unknown user program %s\n", name);
        while (1)
            ;
    }
    return PHYSICAL_ADDR((uint64_t)(&user_program_start) + offset);
}

struct ret_info syscall(uint64_t syscall_num, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t sp)
{
    uint64_t *sp_ptr = (uint64_t *)(sp);

    struct ret_info ret;
    switch (syscall_num)
    {
    case SYS_GETPID:
    {
        ret.a0 = getpid();
        sp_ptr[4] = ret.a0;
        sp_ptr[16] += 4;
        break;
    }
    case SYS_READ:
    {
        ret.a0 = getchar();
        sp_ptr[4] = ret.a0;
        sp_ptr[16] += 4;
        break;
    }
    case SYS_FORK:
    {
        // TODO:
        // 1. create new task and set counter, priority and pid (use our task array)
        int i = 0;
        for (; i < NR_TASKS && task[i]; i++)
            ;
        task[i] = (struct task_struct *)(VIRTUAL_ADDR(alloc_page()));
        task[i]->state = TASK_RUNNING;
        task[i]->counter = 1000;
        task[i]->priority = 20;
        task[i]->pid = i;
        // 2. create root page table, set current process's satp
        uint64_t root_page_table = alloc_page();
        task[i]->satp = 8ull << 60 | (((uint64_t)(task[i]->pid)) << 44) | root_page_table >> 12; // Mode(Sv39) | ASID(PID) | PPN (root_page_table >> 12)
        //   2.1 copy current process's user program address, create mapping for user program
        task[i]->mm.user_program_start = current->mm.user_program_start;
        create_mapping((uint64_t *)root_page_table, 0x1000000, task[i]->mm.user_program_start, PAGE_SIZE, PTE_V | PTE_R | PTE_X | PTE_U | PTE_W);
        //   2.2 create mapping for kernel address
        create_mapping((uint64_t *)root_page_table, 0xffffffc000000000, 0x80000000, 16 * 1024 * 1024, PTE_V | PTE_R | PTE_W | PTE_X);
        create_mapping((uint64_t *)root_page_table, 0xffffffc000000000, 0x80000000, PHYSICAL_ADDR((uint64_t)&rodata_start) - 0x80000000, PTE_V | PTE_R | PTE_X);
        create_mapping((uint64_t *)root_page_table, (uint64_t)&rodata_start, PHYSICAL_ADDR((uint64_t)&rodata_start), (uint64_t)&data_start - (uint64_t)&rodata_start, PTE_V | PTE_R);
        create_mapping((uint64_t *)root_page_table, (uint64_t)&data_start, PHYSICAL_ADDR((uint64_t)&data_start), (uint64_t)&_end - (uint64_t)&data_start, PTE_V | PTE_R | PTE_W);
        create_mapping((uint64_t *)root_page_table, 0x80000000, 0x80000000, 16 * 1024 * 1024, PTE_V | PTE_R | PTE_W | PTE_X);
        create_mapping((uint64_t *)root_page_table, 0x80000000, 0x80000000, PHYSICAL_ADDR((uint64_t)&rodata_start) - 0x80000000, PTE_V | PTE_R | PTE_X);
        create_mapping((uint64_t *)root_page_table, PHYSICAL_ADDR((uint64_t)&rodata_start), PHYSICAL_ADDR((uint64_t)&rodata_start), (uint64_t)&data_start - (uint64_t)&rodata_start, PTE_V | PTE_R);
        create_mapping((uint64_t *)root_page_table, PHYSICAL_ADDR((uint64_t)&data_start), PHYSICAL_ADDR((uint64_t)&data_start), (uint64_t)&_end - (uint64_t)&data_start, PTE_V | PTE_R | PTE_W);
        //   2.3 create mapping for UART address
        create_mapping((uint64_t *)root_page_table, 0x10000000, 0x10000000, 1 * 1024 * 1024, PTE_V | PTE_R | PTE_W | PTE_X);
        // 3. create user stack, copy current process's user stack and save user stack sp to new_task->sscratch
        uint64_t physical_stack = alloc_page();
        task[i]->mm.user_stack = physical_stack;
        create_mapping((uint64_t *)root_page_table, 0x1001000, physical_stack, PAGE_SIZE, PTE_V | PTE_R | PTE_W | PTE_U);
        memcpy((uint64_t *)physical_stack, (uint64_t *)current->mm.user_stack, PAGE_SIZE);
        task[i]->sscratch = read_csr(sscratch); // stack pointer of father process, now we are in S mode, So sscratch is the stack pointer of user stack.
        // / 4. copy mm struct and create mapping
        task[i]->mm.vm = kmalloc(sizeof(struct vm_area_struct));
        INIT_LIST_HEAD(&(task[i]->mm.vm->vm_list));
        struct vm_area_struct *vma;
        list_for_each_entry(vma, &current->mm.vm->vm_list, vm_list)
        {
            struct vm_area_struct *vma_ = kmalloc(sizeof(struct vm_area_struct)); 
            memcpy(vma_, vma, sizeof(struct vm_area_struct));
            list_add(&(vma_->vm_list), &task[i]->mm.vm->vm_list);
            if (vma->mapped)
            {
                uint64_t pa = alloc_pages((vma->vm_end - vma->vm_start) / PAGE_SIZE);
                memcpy(VIRTUAL_ADDR(pa),(uint64_t *)(vma->vm_start), vma->vm_end - vma->vm_start);
                create_mapping((uint64_t *)root_page_table, vma->vm_start, pa, vma->vm_end - vma->vm_start, vma->vm_flags);
            }
        }
        // 5. prepare a0 = 0 and sepc = sepc + 4 for child process
        sp_ptr[4] = 0;
        sp_ptr[16] += 4;
        // 6. copy sp to new task's kernel stack, set ra and sp for __swictch_to
        memcpy((uint64_t)task[i] + PAGE_SIZE - 8 * 31, (uint64_t)current + PAGE_SIZE - 8 * 31, 8 * 31); // 31 regs, each 64 bits
        task[i]->thread.ra = (uint64_t)&trap_s_bottom;                // switch to trap_s_bottom
        task[i]->thread.sp = (uint64_t)task[i] + PAGE_SIZE - 8 * 31; // to the top of the stack
        // 7. set father sp's a0 = child's pid
        sp_ptr[4] = task[i]->pid;
        break;
    }
    case SYS_EXEC:
    {
        // TODO:
        // 1. free current process vm_area_struct and it's mapping area
        struct vm_area_struct *vma;
        uint64_t root_page_table = (current->satp & ((1ULL << 44) - 1)) << 12;
        list_for_each_entry(vma, &current->mm.vm->vm_list, vm_list)
        {
            if (vma->mapped)
            {
                uint64_t pte = get_pte(root_page_table, vma->vm_start);
                uint64_t pa = (pte >> 10) << 12;
                free_pages(pa);
            }
            create_mapping((uint64_t *)root_page_table, vma->vm_start, 0, vma->vm_end - vma->vm_start, vma->vm_flags);
            list_del(&vma->vm_list);
            kfree(vma);
        }
        // 2. reset user stack, user_program_start
        write_csr(sscratch, 0x1001000 + PAGE_SIZE); // reset sscratch, user stack pointer, no need to memset 0.
        current->mm.user_program_start = get_program_address((char *)arg0);
        // 3. create mapping for new user program address
        create_mapping((uint64_t *)root_page_table, 0x1000000, current->mm.user_program_start, PAGE_SIZE, PTE_V | PTE_R | PTE_X | PTE_U | PTE_W);
        // 4. set sepc = 0x1000000
        sp_ptr[16] = 0x1000000;
        // 5. refresh TLB
        asm volatile("sfence.vma"); // flush TLB
        break;
    }
    case SYS_EXIT:
    {
        // TODO:
        // 1. free current process vm_area_struct and it's mapping area
        // 2. free user stack
        // 3. clear current task, set current task->counter = 0
        // 4. call schedule
        struct vm_area_struct *vma;
        uint64_t root_page_table = (current->satp & ((1ULL << 44) - 1)) << 12;
        list_for_each_entry(vma, &current->mm.vm->vm_list, vm_list)
        {
            if (vma->mapped)
            {
                uint64_t pte = get_pte((uint64_t *)root_page_table, vma->vm_start);
                uint64_t pa = (pte >> 10) << 12;
                free_pages(pa);
            }
            create_mapping((uint64_t *)root_page_table, vma->vm_start, 0, vma->vm_end - vma->vm_start, vma->vm_flags);
            list_del(&vma->vm_list);
            kfree(vma);
        }
        kfree(&(current->mm.vm));
        current->mm.vm = NULL;

        free_pages(current->mm.user_stack);
        current->mm.user_stack = 0;
        current->counter = 0;

        schedule();
        break;
    }
    case SYS_WAIT:
    {
        // TODO:
        // 1. find the process which pid == arg0
        int i = 0;
        for (; i < NR_TASKS && task[i] && task[i]->pid != arg0; i++);
         // 2. if not find
         //   2.1. sepc += 4, return
        if (i == NR_TASKS || !task[i])
        {
            sp_ptr[16] += 4;
            break;
        }
        while (1)
        {
            // 3. if find and counter = 0
            //   3.1. free it's kernel stack and page table
            if (task[i]->counter == 0)
            {

                free_pages(PHYSICAL_ADDR(task[i]));
                free_pages((task[i]->satp & ((1ULL << 44) - 1)) << 12);
                task[i] = NULL;
                sp_ptr[16] += 4;
                break;
            }
            // 4. if find and counter != 0
            else
            {
                //   4.1. change current process's priority
                current->priority = task[i]->priority + 1;
                //   4.2. call schedule to run other process
                schedule();
                //   4.3. goto 1. check again
            }
        }
        break;
    }
    case SYS_WRITE:
    {
        int fd = arg0;
        char *buffer = (char *)arg1;
        int size = arg2;
        if (fd == 1)
        {
            for (int i = 0; i < size; i++)
            {
                putchar(buffer[i]);
            }
        }
        ret.a0 = size;
        sp_ptr[4] = ret.a0;
        sp_ptr[16] += 4;
        break;
    }
    case SYS_MMAP:
    {
        struct vm_area_struct *vma = (struct vm_area_struct *)kmalloc(sizeof(struct vm_area_struct));
        if (vma == NULL)
        {
            ret.a0 = -1;
            break;
        }
        vma->vm_start = arg0;
        vma->vm_end = arg0 + arg1;
        vma->vm_flags = arg2;
        vma->mapped = 0;
        list_add(&(vma->vm_list), &(current->mm.vm->vm_list));

        ret.a0 = vma->vm_start;
        sp_ptr[16] += 4;
        break;
    }
    case SYS_MUNMAP:
    {
        ret.a0 = -1;
        struct vm_area_struct *vma;
        list_for_each_entry(vma, &current->mm.vm->vm_list, vm_list)
        {
            if (vma->vm_start == arg0 && vma->vm_end == arg0 + arg1)
            {
                if (vma->mapped == 1)
                {
                    uint64_t pte = get_pte((current->satp & ((1ULL << 44) - 1)) << 12, vma->vm_start);
                    free_pages((pte >> 10) << 12);
                }
                create_mapping((current->satp & ((1ULL << 44) - 1)) << 12, vma->vm_start, 0, (vma->vm_end - vma->vm_start), 0);
                list_del(&(vma->vm_list));
                kfree(vma);

                ret.a0 = 0;
                break;
            }
        }
        // flash the TLB
        asm volatile("sfence.vma");
        sp_ptr[16] += 4;
        break;
    }
    default:
        printf("Unknown syscall! syscall_num = %d\n", syscall_num);
        while (1)
            ;
        break;
    }
    return ret;
}