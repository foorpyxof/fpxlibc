////////////////////////////////////////////////////////////////
//  "string.s"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

.intel_syntax noprefix

.SECTION .note.GNU-stack
.SECTION .text
.GLOBAL fpx_getstringlength
.GLOBAL fpx_strcpy

fpx_getstringlength:
	# modified registers:
	# - rax
	# - r10
	
	xor   rax, rax
	test  rdi, rdi
  jnz   fpx_strlen_start  # if passed string is NOT nullptr
  ret   # else, return 0
	
  fpx_strlen_loop:
  inc   rax

	fpx_strlen_start:
	mov   r10b, BYTE PTR [rdi + rax]
	test  r10b, r10b
	jnz   fpx_strlen_loop
  ret

fpx_strcpy:
  # modified registers:
  # - rax
  # - rsi, rdi
  # - rdx
  # - r9, r10, r11

  # rdi contains destination
  # rsi contains source

  mov   r9, rdi  # safekeeping for returning the destination pointer
  mov   rdi, rsi
  call  fpx_getstringlength
  mov   rdi, r9
  mov   rdx, rax # move string length into 3rd arg to fpx_memcpy
  call  fpx_memcpy

  ret
