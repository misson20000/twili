;amd64_syscalls v2
;
;This is assembly glue to allow C programs to call syscalls without 
;needing a C library to help
;
;v2 resolves the issue of needing to call 6 argument syscalls like mmap
;using an array of longs, instead, syscall6 can be called
;please note that __syscall can not be used with 6 argument syscalls

section .text
	global __syscall, syscall0, syscall1, syscall2, syscall3, syscall4, syscall5, syscall6, syscall_list

	syscall_list:
		mov			rax,rdi			;syscall number
		mov			rdi,[rsi]		;arg1
		mov			rdx,[rsi+16]	;arg3
		mov			r10,[rsi+24]	;arg4
		mov			r8,[rsi+32]		;arg5
		mov			r9,[rsi+40]		;arg6
		mov			rsi,[rsi+8]		;arg2
		syscall						;
		ret							;

	syscall0:
		mov			rax,rdi			;syscall number
		syscall						;
		ret							;

	syscall1:
		mov			rax,rdi			;syscall number
		mov			rdi,rsi			;arg1
		mov			rsi,rdx			;arg2
		syscall						;
		ret							;

	syscall2:
		mov			rax,rdi			;syscall number
		mov			rdi,rsi			;arg1
		mov			rsi,rdx			;arg2
		syscall						;
		ret							;

	syscall3:
		mov			rax,rdi			;syscall number
		mov			rdi,rsi			;arg1
		mov			rsi,rdx			;arg2
		mov			rdx,rcx			;arg3
		syscall						;
		ret							;

	syscall4:
		mov			rax,rdi			;syscall number
		mov			rdi,rsi			;arg1
		mov			rsi,rdx			;arg2
		mov			rdx,rcx			;arg3
		mov			r10,r8			;arg4
		syscall						;
		ret							;

	syscall5:
	__syscall:
		mov			rax,rdi			;syscall number
		mov			rdi,rsi			;arg1
		mov			rsi,rdx			;arg2
		mov			rdx,rcx			;arg3
		mov			r10,r8			;arg4
		mov			r8,r9			;arg5
		syscall						;
		ret							;

	syscall6:
		mov			rax,rdi			;syscall number
		mov			rdi,rsi			;arg1
		mov			rsi,rdx			;arg2
		mov			rdx,rcx			;arg3
		mov			r10,r8			;arg4
		mov			r8,r9			;arg5
		mov			r9,[rsp+8]		;arg6
		syscall						;
		ret							;
