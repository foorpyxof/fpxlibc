////////////////////////////////////////////////////////////////
//  Part of fpxlibc (https://github.com/foorpyxof/fpxlibc)    //
//  Author: Erynn 'foorpyxof' Scholtes                        //
////////////////////////////////////////////////////////////////

# fpx_c-utils/x86_64/endian.Si

.intel_syntax noprefix

.SECTION .note.GNU-stack
.SECTION .text
.GLOBAL fpx_endian_swap

# modified registers:
# - r8
# - r9
# - r10b
fpx_endian_swap:
  test  rdi, rsi  # check for zero/nullptr in either argument
  jz    return
  
  push  rbp
  mov   rbp, rsp
  sub   rsp, 0x2

  mov   r8, rbp
  mov   r9, rdi   # save for later

  store_loop:
  dec   r8
  mov   r10b, BYTE PTR [rdi]
  mov   BYTE PTR [r8], r10b
  inc   rdi
  cmp   r8, rsp
  jne   store_loop

  copy_loop:
  mov   r10b, BYTE PTR [r8]
  mov   BYTE PTR [r9], r10b
  inc   r8
  inc   r9
  cmp   r8, rbp
  jne   copy_loop

  mov   rsp, rbp
  pop   rbp
  return:
  ret

# store input-object to stack [ rbp - bytes(arg2) ]
# start register1 at 0
# [ rbp + register1 ] -> [ rsi + (bytes-resister1-1) ]
# increase register1
