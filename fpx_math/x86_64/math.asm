;
;  "math.asm"
;  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
;  Author: Erynn 'foorpyxof' Scholtes
;


; general math functions
; C header file: ../math.h

section .text
  global fpx_pow
  global fpx_abs

fpx_pow:
  ; modified registers:
  ; - rax (contains return value)
  ; - rdi, rsi
  ; - r10, r11

  ; if power = 0, return 1;
  cmp   rsi, 0
  jz    return_one

  mov   r11, rdi

  ja    .subber
  jb    .adder

  .adder:
  mov   r10, 1
  jmp   .loop_start

  .subber:
  mov   r10, -1

  .loop_start:
  add   rsi, r10
  test  rsi, rsi
  jz    .result
  imul  rdi, r11  ; multiply current iteration of result with its base
  jmp   .loop_start

  .result:
  mov   rax, rdi
  ret

return_one:
  mov   rax, 1
  ret
