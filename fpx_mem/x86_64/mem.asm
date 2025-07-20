;
;  "mem.asm"
;  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
;  Author: Erynn 'foorpyxof' Scholtes
;


; simple mem operations

section .text
  global fpx_memcpy
  global fpx_memset

fpx_memcpy:
  ; modified registers:
  ; - rax (contains return value)
  ; - rsi, rdi
  ; - rdx
  ; - r10, r11

  ; arguments
  ; - pointer to destination
  ; - pointer to source
  ; - how many bytes
  
  ; returns pointer to destination

  mov   rax, rdi    ; keep dst address in rax for returning
  xor   r11, r11    ; set r11 to zero to not interfere with the addresses and such
                    ; during the first iteration of the loop
  
  .start:
  sub   rdx, r11    ; this is here for after every copy operation
  add   rdi, r11    ; to decrease the remaining length and increase
  add   rsi, r11    ; both the 'dst' and 'src' addresses

  cmp   rdx, 0x7
  ja    .qword      ; if remaining length greater than 0x7
  cmp   rdx, 0x3
  ja    .dword      ; else if greater than 0x3
  cmp   rdx, 0x1
  ja    .word       ; else if greater than 0x1
  je    .byte       ; else if equal to 0x1
  ret               ; else return (nothing left to go)

  .qword:
  mov   r10, QWORD [rsi]
  mov   QWORD [rdi], r10
  mov   r11, 0x8    ; move qword size into r11 as prep for updating counters and ptrs
  jmp   .start

  .dword:
  mov   r10d, DWORD [rsi]
  mov   DWORD [rdi], r10d
  mov   r11, 0x4    ; idem
  jmp   .start

  .word:
  mov   r10w, WORD [rsi]
  mov   WORD [rdi], r10w
  mov   r11, 0x2    ; idem
  jmp   .start

  .byte:
  mov   r10b, BYTE [rsi]
  mov   BYTE [rdi], r10b
  mov   r11, 0x1    ; idem
  jmp   .start

fpx_memset:
  ; modified registers:
  ; - rax (contains return value)
  ; - rsi, rdi
  ; - rdx
  ; - r10

  ; arguments:
  ; - pointer to mem address
  ; - byte value
  ; - length

  mov   r10, rsi
  shl   r10, 0x08
  or    rsi, r10
  mov   r10, rsi
  shl   r10, 0x10
  or    rsi, r10
  mov   r10, rsi
  shl   r10, 0x20
  or    rsi, r10
  ; assume rsi was 0x12 at the start;
  ; now rsi is 0x1212121212121212
  ; all bytes in the register are set to whatever was in rsi

  mov   rax, rdi
  xor   r11, r11
  
  .start:
  sub   rdx, r11
  add   rdi, r11
  
  ; check fpx_memset comments for explanation of this
  cmp   rdx, 0x7
  ja    .qword
  cmp   rdx, 0x3
  ja    .dword
  cmp   rdx, 0x1
  ja    .word
  je    .byte
  ret

  .qword:
  mov   QWORD [rdi], rsi
  mov   r11, 0x8
  jmp   .start

  .dword:
  mov   DWORD [rdi], esi
  mov   r11, 0x4
  jmp   .start

  .word:
  mov   WORD [rdi], si
  mov   r11, 0x2
  jmp   .start

  .byte:
  mov   BYTE [rdi], sil
  mov   r11, 0x1
  jmp   .start
