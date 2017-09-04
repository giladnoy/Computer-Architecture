
section .data
;gen: DD 0 ;1st arg = number of generations ;TODO return these variables after finding a way to get the args from the stack
;cycle: DD 0	;2nd arg = cycles between prints

corNum: DD 0
gen: DD 0
cycle: DD 0
        global scheduler
        extern resume, end_co, WorldWidth, WorldLength
    
        


section .text

scheduler:
	mov eax, dword [esp]
	mov dword [gen], eax
	mov eax, dword [esp+4]
	mov dword [cycle], eax
	xor eax, eax
	mov al, byte [WorldLength]
	imul byte [WorldWidth]
	add eax, 2
	mov dword [corNum], eax
	xor edx, edx ;counter for cycle
	mov ecx, dword [esp]
.preNext1:
		mov esi, 2
.next1:
        mov ebx, esi
        call resume
        inc esi
        inc edx
        cmp edx, dword [cycle]
        jl .noPrint
        xor edx, edx
        mov ebx, 1
        call resume
.noPrint: 
        cmp esi, dword [corNum]
        jl .next1
        jmp .preNext2
.preNext2:
        mov esi, 2        
.next2:
        mov ebx, esi
		call resume
        inc esi
        inc edx
        cmp edx, dword [cycle]
        jl .noPrint2
        xor edx, edx
        mov ebx, 1
        call resume
.noPrint2:        
	cmp esi, [corNum]
	jl .next2
    dec ecx
    cmp ecx, 0
    jg .preNext1
    mov ebx, 1
    call resume             ;call for last print
    call end_co             ; stop co-routines