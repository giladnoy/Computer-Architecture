section .data
Msg:
    DB "Hello, Infected File", 10, 0

section .text

SYS_READ equ 3 ; arg2 file descriptor, arg3 pointer to input buffer, arg4 buffer size, max, returns number of bytes received */
SYS_WRITE equ 4 ; arg2 file descriptor, arg3 pointer to output buffer, arg4 count of bytes to send, returns number of bytes sent*/
STDIN equ 0
STDOUT equ 1
STDERR equ 2
OPEN equ 5 ; arg2 the file pathname, arg3 file access flags, arg4 file permission (0777, 0644)*/
O_WRONLY equ 1
O_APPEND equ 1024
O_RDONLY equ 0
CLOSE equ 6 ;arg2 file descriptor

global _start
global system_call
global infection
global infector
extern main
_start:
    pop    dword ecx    ; ecx = argc
    mov    esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx     ; put the number of arguments into eax
    shl     eax,2       ; compute the size of argv in bytes
    add     eax,esi     ; add the size to the address of argv 
    add     eax,4       ; skip NULL at the end of argv
    push    dword eax   ; char *envp[]
    push    dword esi   ; char* argv[]
    push    dword ecx   ; int argc

    call    main        ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,1
    int     0x80
    nop
        
system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller 

code_start:
    
infection:
    push    ebp     ; push start location of previous 
    mov ebp, esp    ; Entry code - set up ebp and esp
    pushad          ; Save registers
    push dword 21   ; push arg4 - string length
    push Msg       ; push arg3 - string
    push STDOUT 
    push SYS_WRITE
    call system_call
    add esp, 4*4    ; remove 4 args from stack
    popad           ; Restore registers
    ;mov esp, ebp    ; Function exit code
    pop ebp
    ret

infector:
    push    ebp         ; push start location of previous 
    mov ebp, esp        ; Entry code - set up ebp and esp
    pushad              ; Save registers

    ;open file:
    push dword 0777     ; push permission
    push dword O_APPEND + O_WRONLY     ; 1024 flag for appending text to file, 1 flag for writing
    push dword [ebp + 8]; push file name
    push OPEN
    call system_call    ; eax now holds int value of file
    add esp, 4*4
    mov edx, eax       ; save returned value in edx

    ;append executable code:
    push dword (code_end-code_start)
    push dword code_start
    push edx
    push SYS_WRITE
    call system_call
    add esp, 4*4

    ;close file:
    push edx
    push CLOSE
    call system_call
    add esp, 2*4

    ;end function:
    popad               ; Restore registers
    ;mov esp, ebp        ; Function exit code
    pop ebp
    ret


code_end: