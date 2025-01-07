////////////////////////////////////////////////////////////////
//  "format.s"                                                //
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

.intel_syntax noprefix

.SECTION .note.GNU-stack
.SECTION .text
.GLOBAL fpx_strint

fpx_strint:
  # modified registers:
  # - rax (return value)
  # - rdi, sil
  # - rdx

  mov   rax, 0

  # check for negative
  cmp   BYTE PTR [rdi], '-'
  jne   strint_check
  mov   sil, 0x0  # not positive; negative number
  inc   rdi

  strint_check:
  cmp   BYTE PTR [rdi], '0'
  jb    strint_end
  cmp   BYTE PTR [rdi], '9'
  ja    strint_end

  xor   rdx, rdx

  strint_logic:
  imul  rax, 10
  mov   dl, BYTE PTR [rdi]
  sub   rdx, '0'
  test  sil, sil
  jz    strint_neg

  add   rax, rdx
  jmp strint_next

  strint_neg:
  sub   rax, rdx

  strint_next:
  inc   rdi
  jmp   strint_check

  strint_end:
  ret
