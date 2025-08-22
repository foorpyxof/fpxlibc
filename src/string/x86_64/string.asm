;
;  "string.asm"
;  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
;  Author: Erynn 'foorpyxof' Scholtes
;

; .intel_syntax noprefix

section .text
global fpx_getstringlength
global fpx_strcpy

extern fpx_memcpy

fpx_getstringlength:
	; modified registers:
	; - rax
	; - r10
	
	xor   rax, rax
	test  rdi, rdi
  jnz   .start  ; if passed string is NOT nullptr
  ret   ; else, return 0
	
  .loop:
  inc   rax

	.start:
	mov   r10b, BYTE [rdi + rax]
	test  r10b, r10b
	jnz   .loop
  ret

fpx_strcpy:
  ; modified registers:
  ; - rax
  ; - rsi, rdi
  ; - rdx
  ; - r9, r10, r11

  ; rdi contains destination
  ; rsi contains source

  mov   r9, rdi  ; safekeeping for returning the destination pointer
  mov   rdi, rsi
  call  fpx_getstringlength
  mov   rdi, r9
  mov   rdx, rax ; move string length into 3rd arg to fpx_memcpy
  call  fpx_memcpy

  ret
