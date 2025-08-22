;
;  "format.asm"
;  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
;  Author: Erynn 'foorpyxof' Scholtes
;

section .text
global fpx_strint

fpx_strint:
  ; modified registers:
  ; - rax (return value)
  ; - rdi, sil
  ; - rdx

  mov   rax, 0
  mov   sil, 1    ; assume positive

  ; check for negative
  cmp   BYTE [rdi], '-'
  jne   .check
  mov   sil, 0x0  ; not positive; negative number
  inc   rdi

  .check:
  cmp   BYTE [rdi], '0'
  jb    .end
  cmp   BYTE [rdi], '9'
  ja    .end

  xor   rdx, rdx

  .logic:
  imul  rax, 10
  mov   dl, BYTE [rdi]
  sub   rdx, '0'
  test  sil, sil
  jz    .neg

  add   rax, rdx
  jmp .next

  .neg:
  sub   rax, rdx

  .next:
  inc   rdi
  jmp   .check

  .end:
  ret
