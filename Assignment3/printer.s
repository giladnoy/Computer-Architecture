STDIN equ 0
STDOUT equ 1
STDERR equ 2
SYS_EXIT equ 1
SYS_READ equ 3 ; arg2 file descriptor, arg3 pointer to input buffer, arg4 buffer size, max, returns number of bytes received */
SYS_WRITE equ 4 ; arg2 file descriptor, arg3 pointer to output buffer, arg4 count of bytes to send, returns number of bytes sent*/
SYS_OPEN equ 5 ; arg2 the file pathname, arg3 file access flags, arg4 file permission (0777, 0644)*/
SYS_CLOSE equ 6 ;arg2 file descriptor

%macro system_call 4
        push ebx
        push ecx
        push edx
        push edi
        push esi
        mov eax, %1
        mov ebx, %2
        mov ecx, %3
        mov edx, %4
        int 80h
        pop esi
        pop edi
        pop edx
        pop ecx
        pop ebx
%endmacro

        global printer
        extern resume, WorldLength, WorldWidth, state, cellNum

        ;; /usr/include/asm/unistd_32.h
sys_write:      equ   4
stdout:         equ   1


section .data


temp: DB 0
newline: DB 10,0
section .text

printer:
        mov esi, 0
        mov bl, 0
        mov edx, dword [cellNum]
        mov bh, byte [WorldWidth]
        dec bh
.print_loop:
        mov cl, byte [state+esi]
        mov byte [temp], cl        
        system_call SYS_WRITE, STDOUT, temp, 1
        inc esi;total cells
        inc bl;width
        cmp bl, bh
        jle .print_loop
.nextLine:
        mov bl, 0        
        system_call SYS_WRITE, STDOUT, newline, 1
        cmp esi, edx
        jge .resume_scheduler
        jmp .print_loop

.resume_scheduler:      
        xor ebx, ebx
        call resume             ; resume scheduler
        jmp printer