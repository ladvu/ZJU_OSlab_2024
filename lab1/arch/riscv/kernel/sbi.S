	.file	"sbi.c"
	.option nopic
	.attribute arch, "rv64i2p0_m2p0_a2p0_f2p0_d2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.sbi_call,"ax",@progbits
	.align	2
	.globl	sbi_call
	.type	sbi_call, @function
sbi_call:
.LFB0:
	.file 1 "sbi.c"
	.loc 1 5 39
	.cfi_startproc
	addi	sp,sp,-128
	.cfi_def_cfa_offset 128
	sd	s0,120(sp)
	sd	s1,112(sp)
	sd	s2,104(sp)
	sd	s3,96(sp)
	.cfi_offset 8, -8
	.cfi_offset 9, -16
	.cfi_offset 18, -24
	.cfi_offset 19, -32
	addi	s0,sp,128
	.cfi_def_cfa 8, 0
	sd	a0,-72(s0)
	sd	a1,-80(s0)
	sd	a2,-88(s0)
	sd	a3,-96(s0)
	sd	a4,-104(s0)
	sd	a5,-112(s0)
	sd	a6,-120(s0)
	sd	a7,-128(s0)
	.loc 1 8 3
	ld	t3,-72(s0)
	ld	t4,-80(s0)
	ld	t5,-88(s0)
	ld	t6,-96(s0)
	ld	t0,-104(s0)
	ld	s1,-112(s0)
	ld	s2,-120(s0)
	ld	s3,-128(s0)
 #APP
# 8 "sbi.c" 1
	mv a7, t3
mv a6, t4
mv a0, t5
mv a1, t6
mv a2, t0
mv a3, s1
mv a4, s2
mv a5, s3
ecall
mv t4, a0
mv t3, a1

# 0 "" 2
 #NO_APP
	sd	t4,-64(s0)
	sd	t3,-56(s0)
	.loc 1 27 10
	ld	a5,-64(s0)
	sd	a5,-48(s0)
	ld	a5,-56(s0)
	sd	a5,-40(s0)
	ld	a4,-48(s0)
	ld	a5,-40(s0)
	mv	t1,a4
	mv	t2,a5
	mv	a4,t1
	mv	a5,t2
	.loc 1 28 1
	mv	a0,a4
	mv	a1,a5
	ld	s0,120(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 128
	ld	s1,112(sp)
	.cfi_restore 9
	ld	s2,104(sp)
	.cfi_restore 18
	ld	s3,96(sp)
	.cfi_restore 19
	addi	sp,sp,128
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE0:
	.size	sbi_call, .-sbi_call
	.text
.Letext0:
	.file 2 "/home/oslab/os_experiment/lab1/include/defs.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x155
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.byte	0x1
	.4byte	.LASF18
	.byte	0xc
	.4byte	.LASF19
	.4byte	.LASF20
	.4byte	.Ldebug_ranges0+0
	.8byte	0
	.4byte	.Ldebug_line0
	.byte	0x2
	.byte	0x4
	.byte	0x5
	.string	"int"
	.byte	0x3
	.byte	0x1
	.byte	0x8
	.4byte	.LASF0
	.byte	0x3
	.byte	0x1
	.byte	0x8
	.4byte	.LASF1
	.byte	0x3
	.byte	0x2
	.byte	0x5
	.4byte	.LASF2
	.byte	0x3
	.byte	0x2
	.byte	0x7
	.4byte	.LASF3
	.byte	0x3
	.byte	0x4
	.byte	0x7
	.4byte	.LASF4
	.byte	0x3
	.byte	0x8
	.byte	0x5
	.4byte	.LASF5
	.byte	0x4
	.4byte	.LASF21
	.byte	0x2
	.byte	0x17
	.byte	0x1c
	.4byte	0x66
	.byte	0x3
	.byte	0x8
	.byte	0x7
	.4byte	.LASF6
	.byte	0x3
	.byte	0x1
	.byte	0x6
	.4byte	.LASF7
	.byte	0x3
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.byte	0x3
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.byte	0x5
	.4byte	.LASF22
	.byte	0x10
	.byte	0x2
	.byte	0x41
	.byte	0x8
	.4byte	0xaa
	.byte	0x6
	.4byte	.LASF10
	.byte	0x2
	.byte	0x42
	.byte	0xc
	.4byte	0x5a
	.byte	0
	.byte	0x6
	.4byte	.LASF11
	.byte	0x2
	.byte	0x43
	.byte	0xc
	.4byte	0x5a
	.byte	0x8
	.byte	0
	.byte	0x7
	.4byte	.LASF23
	.byte	0x1
	.byte	0x3
	.byte	0xf
	.4byte	0x82
	.8byte	.LFB0
	.8byte	.LFE0-.LFB0
	.byte	0x1
	.byte	0x9c
	.byte	0x8
	.string	"ext"
	.byte	0x1
	.byte	0x3
	.byte	0x21
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xb8,0x7f
	.byte	0x8
	.string	"fid"
	.byte	0x1
	.byte	0x3
	.byte	0x2f
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xb0,0x7f
	.byte	0x9
	.4byte	.LASF12
	.byte	0x1
	.byte	0x3
	.byte	0x3d
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xa8,0x7f
	.byte	0x9
	.4byte	.LASF13
	.byte	0x1
	.byte	0x3
	.byte	0x4c
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0xa0,0x7f
	.byte	0x9
	.4byte	.LASF14
	.byte	0x1
	.byte	0x4
	.byte	0x21
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x98,0x7f
	.byte	0x9
	.4byte	.LASF15
	.byte	0x1
	.byte	0x4
	.byte	0x30
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x90,0x7f
	.byte	0x9
	.4byte	.LASF16
	.byte	0x1
	.byte	0x4
	.byte	0x3f
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x88,0x7f
	.byte	0x9
	.4byte	.LASF17
	.byte	0x1
	.byte	0x5
	.byte	0x21
	.4byte	0x5a
	.byte	0x3
	.byte	0x91
	.byte	0x80,0x7f
	.byte	0xa
	.string	"ret"
	.byte	0x1
	.byte	0x6
	.byte	0x11
	.4byte	0x82
	.byte	0x2
	.byte	0x91
	.byte	0x40
	.byte	0
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.byte	0x1
	.byte	0x11
	.byte	0x1
	.byte	0x25
	.byte	0xe
	.byte	0x13
	.byte	0xb
	.byte	0x3
	.byte	0xe
	.byte	0x1b
	.byte	0xe
	.byte	0x55
	.byte	0x17
	.byte	0x11
	.byte	0x1
	.byte	0x10
	.byte	0x17
	.byte	0
	.byte	0
	.byte	0x2
	.byte	0x24
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x3
	.byte	0x8
	.byte	0
	.byte	0
	.byte	0x3
	.byte	0x24
	.byte	0
	.byte	0xb
	.byte	0xb
	.byte	0x3e
	.byte	0xb
	.byte	0x3
	.byte	0xe
	.byte	0
	.byte	0
	.byte	0x4
	.byte	0x16
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x5
	.byte	0x13
	.byte	0x1
	.byte	0x3
	.byte	0xe
	.byte	0xb
	.byte	0xb
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x1
	.byte	0x13
	.byte	0
	.byte	0
	.byte	0x6
	.byte	0xd
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x38
	.byte	0xb
	.byte	0
	.byte	0
	.byte	0x7
	.byte	0x2e
	.byte	0x1
	.byte	0x3f
	.byte	0x19
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x27
	.byte	0x19
	.byte	0x49
	.byte	0x13
	.byte	0x11
	.byte	0x1
	.byte	0x12
	.byte	0x7
	.byte	0x40
	.byte	0x18
	.byte	0x97,0x42
	.byte	0x19
	.byte	0
	.byte	0
	.byte	0x8
	.byte	0x5
	.byte	0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0x9
	.byte	0x5
	.byte	0
	.byte	0x3
	.byte	0xe
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0xa
	.byte	0x34
	.byte	0
	.byte	0x3
	.byte	0x8
	.byte	0x3a
	.byte	0xb
	.byte	0x3b
	.byte	0xb
	.byte	0x39
	.byte	0xb
	.byte	0x49
	.byte	0x13
	.byte	0x2
	.byte	0x18
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x2c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.LFB0
	.8byte	.LFE0-.LFB0
	.8byte	0
	.8byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.8byte	.LFB0
	.8byte	.LFE0
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF12:
	.string	"arg0"
.LASF15:
	.string	"arg3"
.LASF19:
	.string	"sbi.c"
.LASF16:
	.string	"arg4"
.LASF11:
	.string	"value"
.LASF21:
	.string	"uint64_t"
.LASF6:
	.string	"long long unsigned int"
.LASF13:
	.string	"arg1"
.LASF14:
	.string	"arg2"
.LASF18:
	.string	"GNU C17 10.1.0 -march=rv64imafd -mabi=lp64 -mcmodel=medany -mtune=rocket -g -ffunction-sections -fdata-sections -fno-builtin"
.LASF23:
	.string	"sbi_call"
.LASF17:
	.string	"arg5"
.LASF9:
	.string	"long unsigned int"
.LASF3:
	.string	"short unsigned int"
.LASF22:
	.string	"sbiret"
.LASF1:
	.string	"unsigned char"
.LASF4:
	.string	"unsigned int"
.LASF0:
	.string	"char"
.LASF10:
	.string	"error"
.LASF5:
	.string	"long long int"
.LASF2:
	.string	"short int"
.LASF8:
	.string	"long int"
.LASF7:
	.string	"signed char"
.LASF20:
	.string	"/home/oslab/os_experiment/lab1/arch/riscv/kernel"
	.ident	"GCC: (GNU) 10.1.0"
