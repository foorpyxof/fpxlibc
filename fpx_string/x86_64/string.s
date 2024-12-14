////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

# fpx_string/x86_64/string.S

.intel_syntax noprefix

.SECTION .note.GNU-stack
.SECTION .text
.GLOBAL fpx_getstringlength

fpx_getstringlength:
	# modified registers:
	# - rax
	# - r10
	
	xor		rax, rax
	test	rdi, rdi
  jnz		fpx_strlen_start  # if passed string is NOT nullptr
  ret   # else, return 0
	
  fpx_strlen_loop:
  inc   rax

	fpx_strlen_start:
	mov		r10b, BYTE PTR [rdi + rax]
	test	r10b, r10b
	jnz		fpx_strlen_loop
  ret
