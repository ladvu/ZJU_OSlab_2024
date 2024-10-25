#ifndef PRINT_ONLY
.section .text.entry

.global trap_s
.extern handler_s
.equ reg_size, 0x8
.align 2

trap_s:
	# TODO: save the caller saved registers and sepc
	addi sp,sp,-104
	sd s0,0(sp)
	sd s1,8(sp)
	sd s2,16(sp)
	sd s3,24(sp)
	sd s4,32(sp)
	sd s5,40(sp)
	sd s6,48(sp)
	sd s7,56(sp)
	sd s8,64(sp)
	sd s9,72(sp)
	sd s10,80(sp)
	sd s11,88(sp)
	csrr t0, sepc
	sd t0,96(sp)
	# TODO: call handler_s(scause)
	csrr t0, scause 
	mv a0,t0
	call handler_s	
	# TODO: load sepc and caller saved registers
	ld s0,0(sp)
	ld s1,8(sp)
	ld s2,16(sp)
	ld s3,24(sp)
	ld s4,32(sp)
	ld s5,40(sp)
	ld s6,48(sp)
	ld s7,56(sp)
	ld s8,64(sp)
	ld s9,72(sp)
	ld s10,80(sp)
	ld s11,88(sp)
	ld t0,96(sp)
	csrw sepc, t0
	addi sp,sp,104
	
	sret

#endif