
;;; This is a simplified co-routines implementation:
;;; CORS contains just stack tops, and we always work
;;; with co-routine indexes.
        global init_co, start_co, end_co, resume
        extern WorldWidth, WorldLength, gen, cycle

maxcors:        equ 100*100+2         ; maximum number of co-routines
stacksz:        equ 16*1024     ; per-co-routine stack size


section .bss

stacks: resb maxcors * stacksz  ; co-routine stacks
cors:   resd maxcors            ; simply an array with co-routine stack tops
curr:   resd 1                  ; current co-routine
origsp: resd 1                  ; original stack top
tmp:    resd 1                  ; temporary value

section .data
xgen: DD 0
ycycle: DD 0
section .text

        ;; ebx = co-routine index to initialize
        ;; edx = co-routine start
        ;; other registers will be visible to co-routine after "start_co"
init_co:
        push eax                ; save eax (on caller's stack)
		;pushad
		push edx                ;*edx is pushed as it is affected by imul
		mov edx,0
		mov eax,stacksz
        imul ebx	          ; eax = co-routine's stack offset in stack
		add eax, stacks + stacksz ; eax = top of (empty) co-routine's stack
        mov [cors + ebx*4], eax ; store co-routine's stack top
        cmp ebx, 2
        jl .not_cell
        xor eax, eax
        mov ax, bx
        sub ax, 2
        idiv byte [WorldWidth] ;divide cell by num of columns = al holds the row (Quotient)
        xor edx, edx
        add dl, al
        mov dword [ycycle], edx
        xor edx, edx
        add dl, ah
        mov dword [xgen], edx
        jmp .finish_init

.not_cell:
        cmp ebx, 0
        jg .finish_init
        mov eax, dword [gen]
        mov dword [xgen], eax
        mov eax, dword [cycle]
        mov dword [ycycle], eax
        

.finish_init:
		pop edx
		pop eax					; restore eax (from caller's stack)
        mov [tmp], esp          ; save caller's stack top
        mov esp, [cors + ebx*4] ; esp = co-routine's stack top
        push dword [ycycle]
        push dword [xgen]
        push edx                 ; save return address to co-routine stack
        pushfd                   ; save flags
        pushad                   ; save all registers        
        mov [cors + ebx*4], esp ; update co-routine's stack top
        mov esp, [tmp]          ; restore caller's stack top
        ret                     ; return to caller

        ;; ebx = co-routine index to start
start_co:
        pushfd
        pushad                  ; save all registers (restored in "end_co")
        mov [origsp], esp       ; save caller's stack top
        mov [curr], ebx         ; store current co-routine index
        jmp resume.cont         ; perform state-restoring part of "resume"

        ;; can be called or jumped to
end_co:
        mov esp, [origsp]       ; restore stack top of whoever called "start_co"
        popad                    ; restore all registers
        popfd
        ret                     ; return to caller of "start_co"

        ;; ebx = co-routine index to switch to
resume:                         ; "call resume" pushed return address
        pushfd                  ; save flags to source co-routine stack
        pushad                  ; save all registers
        xchg ebx, [curr]        ; ebx = current co-routine index
        mov [cors + ebx*4], esp ; update current co-routine's stack top
        mov ebx, [curr]         ; ebx = destination co-routine index
.cont:  
        mov esp, [cors + ebx*4] ; get destination co-routine's stack top
        popad                    ; restore all registers
        popfd                    ; restore flags
        ret                     ; jump to saved return address