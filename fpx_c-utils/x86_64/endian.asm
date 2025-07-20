;
;  "endian.asm"
;  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
;  Author: Erynn 'foorpyxof' Scholtes
;


section .text
global fpx_endian_swap

fpx_endian_swap:
  ; modified registers:
  ; - r8
  ; - r9
  ; - r10b

  ; check for zero/nullptr in either argument
  test  rdi, rdi
  jz    .return
  test  rsi, rsi
  jz    .return

  push  rbp
  mov   rbp, rsp
  sub   rsp, rsi

  mov   r8, rbp
  mov   r9, rdi   ; save for later

  .store_loop:
  dec   r8
  mov   r10b, BYTE [rdi]
  mov   BYTE [r8], r10b
  inc   rdi
  cmp   r8, rsp
  jne   .store_loop

  .copy_loop:
  mov   r10b, BYTE [r8]
  mov   BYTE [r9], r10b
  inc   r8
  inc   r9
  cmp   r8, rbp
  jne   .copy_loop

  mov   rsp, rbp
  pop   rbp
  .return:
  ret

; store input-object to stack [ rbp - bytes(arg2) ]
; start register1 at 0
; [ rbp + register1 ] -> [ rsi + (bytes-resister1-1) ]
; increase register1
