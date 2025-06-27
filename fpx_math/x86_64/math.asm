; ////////////////////////////////////////////////////////////////
; //  "math.s"                                                  //
; //  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
; //  Author: Erynn 'foorpyxof' Scholtes                        //
; ////////////////////////////////////////////////////////////////


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

  ja    _fpx_pow_subber
  jb    _fpx_pow_adder

  _fpx_pow_adder:
  mov   r10, 1
  jmp   _fpx_pow_looper

  _fpx_pow_subber:
  mov   r10, -1

  _fpx_pow_looper:
  add   rsi, r10
  test  rsi, rsi
  jz    _fpx_pow_result
  imul  rdi, r11  ; multiply current iteration of result with its base
  jmp   _fpx_pow_looper

  _fpx_pow_result:
  mov   rax, rdi
  ret

return_one:
  mov   rax, 1
  ret
