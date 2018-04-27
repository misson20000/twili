section .text
	global memcpy

	memcpy:					;(rdi,rsi,rdx)
		add rdi,rdx			;move rdi to end of source
		lea rax,[rsi+rdx]	;load end of dest in rax
	memcpy_loop:
		dec	rdx				;decrement rax
		js	memcpy_end		;end if no more data to read
		mov	rsi,[rdi]		;copy one byte
		mov	[rax],rsi		;copy one byte
		dec rdi				;move to next byte
		dec rax				;move to next byte
		jmp	memcpy_loop		;loop
	memcpy_end:
		ret
