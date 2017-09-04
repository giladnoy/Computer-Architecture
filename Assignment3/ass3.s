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

%macro printDebugMsg 2
section .data
        %%string: DB %1,0         
section .text
        cmp byte [d], 0
        je %%no_debug
        pushad
        system_call SYS_WRITE, STDERR, %%string, %2
;        mov eax, SYS_WRITE
;        mov ebx, STDERR
;        mov ecx, %%string
;        mov edx, %2
;        int 80h
        popad
%%no_debug:        
%endmacro

%macro printDebug 2
;section .data
;        %%string: DB %1,10,0         
;section .text
        cmp byte [d], 0
        je %%no_debug
        pushad
        system_call SYS_WRITE, STDERR, %1, %2
;        mov eax, SYS_WRITE
;        mov ebx, STDERR
;        mov ecx, %1
;        mov edx, %2
;        int 80h
        popad
%%no_debug:        
%endmacro
 
%macro printDebugl 2
;section .data
;        %%string: DB %1,10,0         
;section .text
        cmp byte [d], 0
        je %%no_debug
        pushad
        system_call SYS_WRITE, STDERR, %1, %2
;        mov eax, SYS_WRITE
;        mov ebx, STDERR
;        mov ecx, %1
;        mov edx, %2
;        int 80h
        system_call SYS_WRITE, STDERR, newline, 1
;        mov eax, SYS_WRITE
;        mov ebx, STDERR
;        mov ecx, newline
;        mov edx, 1
;        int 80h
        popad
%%no_debug:        
%endmacro

%macro lenFromString 2
%%start:
        mov %1, 1
        cmp byte [%2+1], 0
        je %%finish
        add %1, 1
        cmp byte [%2+2], 0
        je %%finish
        add %1, 1
        cmp byte [%2+3], 0
        je %%finish
        add %1, 1
        cmp byte [%2+4], 0
        je %%finish
        add %1, 1
        cmp byte [%2+5], 0
        je %%finish
        add %1, 1
        cmp byte [%2+6], 0
        je %%finish
        add %1, 1
        jmp %%start
%%finish:       
%endmacro

        global main, WorldLength, WorldWidth, gen, cycle, state, cellNum
        extern init_co, start_co, resume, cell
        extern scheduler, printer
        
section .bss
state: RESB 100*100
buffer: RESB 105*105

section .data
d: DB 0
WorldLength: DB 0
WorldWidth: DB 0
cellNum: DD 0
gen: DD 0                 ;generations
cycle: DD 0                 ;cycles
file: DB 0
ten: dd 10
newline: DB 10,0
temp: DB 0

section .text

main:
        enter 0, 0
        mov ecx, [ebp + 8]      ; ecx = argc
        cmp ecx, 7              ;num of args = 7? (next line)
        mov ecx, [ebp + 12]     ;ecx = argv
        jl .noDebug             ;no  -> no "-d"
        mov byte [d], 1         ;yes -> debug mode 
        add ecx, 4              ;ecx points to argv[1], in case of "-d"
.noDebug:
;assume 1st arg is after "-d" if present
        ;getting the args values
        ;mov [file], esi
        mov esi, dword [ecx+8]
        printDebugMsg "length=", 7       
        lenFromString edi, esi
        printDebugl esi, edi
        push esi
        call atoi
        mov byte [WorldLength], al
        mov esi, dword [ecx+12]
        printDebugMsg "width=", 6
        lenFromString edi, esi
        printDebugl esi, edi
        push esi
        call atoi
        mov byte [WorldWidth], al
        mov esi, dword [ecx+16]
        printDebugMsg "number of generations=", 22
        lenFromString edi, esi
        printDebugl esi, edi
        push esi
        call atoi
        mov dword [gen], eax
        mov esi, dword [ecx+20]
        printDebugMsg "print frequency=", 16
        lenFromString edi, esi
        printDebugl esi, edi
        push esi
        call atoi
        mov dword [cycle], eax
        mov eax, 0
        mov al, byte [WorldWidth] 
        imul byte [WorldLength]
        mov dword [cellNum], eax
        add esp, 16
        mov esi, dword [ecx+4]                                   ;esi holds the number of cells
        system_call SYS_OPEN, esi, 0, 0777   
        mov ebx, eax
        system_call SYS_READ, ebx, buffer, 104*104
        mov ebx, 0;the offset of the buffer
        mov edx, 0;the offset of the state
.init_state:
        mov cl, byte [buffer+ebx]
        cmp cl, '1'
        je .one
        cmp cl, ' '
        je .zero
        cmp cl, 10
        je .next
.one:
        mov byte [state+edx], '1'
        inc edx
        jmp .next
.zero:
        mov byte [state+edx], '0'
        inc edx
        jmp .next
.next:
        inc ebx
        cmp edx, dword [cellNum]
        jl .init_state
        cmp byte [d], 0
        je start_sim
        call print_state
        jmp start_sim

print_state:
        pushad
        pushfd
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
        jg start_sim
        jmp .print_loop
        popfd
        popad
        ret

start_sim:
        xor ebx, ebx            ; scheduler is co-routine 0
        push dword [cycle]
        push dword [gen]
        mov edx, scheduler
        call init_co            ; initialize scheduler state

        inc ebx                 ; printer i co-routine 1
        mov edx, printer
        call init_co            ; initialize printer state

        inc ebx                 ;first cell is co-routine 2
        mov esi, dword [cellNum];esi = number of cells
        add esi, 2              ;esi = number of co-routines

.init_cell:
        mov edx, cell_func
        call init_co
        inc ebx
        cmp ebx, esi            ;current co-routine < number of co-routines?
        jl .init_cell           ;yes -> continue to next cell

        xor ebx, ebx            ; starting co-routine = scheduler
        call start_co           ; start co-routines

.exit:
        mov eax, SYS_EXIT
        xor ebx, ebx
        int 80h

cell_func:
        CALL cell                       ;check condition of neighbors
        push eax
        xor ebx, ebx                    ;set next cor to be scheduler
        CALL resume                     ;end of first phase of cell after checking neighbors
        mov ecx, dword [esp+4]; x value
        mov edx, dword [esp+8]; y value
        xor eax, eax
        mov al, byte [WorldWidth]
        imul edx
        add ecx, eax
        pop eax
        add al, '0'
        mov byte [state + ecx], al  ;put new age in current state
        xor ebx, ebx                    ;transfer control to scheduler
        CALL resume
        jmp cell_func                   ;wake up and start over...

atoi:
        push ebp
        mov ebp, esp        ; Entry code - set up ebp and esp
        push ecx
        push edx
        push ebx
        mov ecx, dword [ebp+8]  ; Get argument (pointer to string)
        xor eax,eax
        xor ebx,ebx
atoi_loop:
        xor edx,edx
        cmp byte[ecx],0
        jz  atoi_end
        imul dword [ten]
        mov bl,byte[ecx]
        sub bl,'0'
        add eax,ebx
        inc ecx
        jmp atoi_loop
atoi_end:
        pop ebx                 ; Restore registers
        pop edx
        pop ecx
        mov     esp, ebp        ; Function exit code
        pop     ebp
        ret        