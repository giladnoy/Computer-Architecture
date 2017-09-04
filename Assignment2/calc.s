;Check validity of operand received from console
%macro jmp_if_operand 3
	cmp byte [%1], %2
	jne %%no_match
	cmp byte [%1 + 1], 10
	je %3
	%%no_match:
%endmacro

;Print string in arg without \n
%macro print 1 
section .data
	%%string: DB %1,0
section .text
	pushad
	pushfd
	push %%string
	push sform
	call printf
	add esp,8
	popfd
	popad
%endmacro

;Print string in arg with \n
%macro printl 1 
section .data
	%%string: DB %1,10,0
section .text
	pushad
	pushfd
	push %%string
	push sform
	call printf
	add esp,8
	popfd
	popad
%endmacro

;Print a 4 byte number received as argument
%macro printn 1 	
	pushad
	pushfd
	push %1
	push nform
	CALL printf
	add esp, 8
	popfd
	popad
%endmacro

;Print a 1 byte hexadecimal number received as argument
%macro printBCDlz 1 
	pushad
	pushfd
	mov byte [temp], %1
	mov byte [temp+1],0
	mov byte [temp+2],0
	mov byte [temp+3],0
	push dword [temp]
	push BCDformLZ
	CALL printf
	add esp, 8
	popfd
	popad
%endmacro

;Print 2 digits in BCD format without printing a leading zero, arg1 - byte size
%macro printBCDnz 1
	pushad
	pushfd 	
	mov byte [temp], %1
	mov byte [temp+1],0
	mov byte [temp+2],0
	mov byte [temp+3],0
	push dword [temp]
	push BCDformNZ
	CALL printf
	add esp, 8
	popfd
	popad
%endmacro

;Call malloc(5) without affecting registers
%macro new_node 0 			
	pushad
	pushfd
	push 5
	CALL malloc
	add esp, 4
	mov dword [temp], eax
	popfd
	popad
	mov eax, dword [temp]
%endmacro

;Increase opearand counter by 1
%macro IncCounter 0
	mov ebx, [counter]
	inc ebx
	mov [counter], ebx
%endmacro

%macro debugTop 0
	cmp byte [debug], 0
	je %%end
	pushad
	pushfd
	push stackdebug
	push dword [stderr]
	CALL fprintf
	add esp, 4*2
	mov bl, '~' 									
	push ebx 										;put ~ in stack to indicate end of printing
	mov ecx, dword [stackSize] 						;put current location of stack in ecx	
	mov edx, dword [stk + (ecx-1)*4] 				;put address of number in the current location of stack in edx
%%push_digits:							;loop of adding digits to stack
	mov bl, byte [edx] 								;put number value of current node in bl
	push ebx										;put number in stack
	cmp dword [edx + 1], 0 							;check if address of next node is 0
	je %%start 										;yes -> start printing digits from stack
	mov edx, [edx + 1]								;no  -> get address of next node and return to beginning of loop
	jmp %%push_digits

%%start:
	pop ebx
	mov byte [temp], bl
	mov byte [temp+1],0
	mov byte [temp+2],0
	mov byte [temp+3],0
	push dword [temp]
	push BCDformLZ
	push dword [stderr]
 	CALL fprintf
 	add esp, 4*3 									;print 2 digits, if 1st digit is zero it won't be printed
%%print_loop:
 	pop ebx 										;pop next 2 digits/finish flag
 	cmp bl, '~' 									;check if popped element is finish flag
 	je %%finish_printing							;yes -> finish printing
 	mov byte [temp], bl
	mov byte [temp+1],0
	mov byte [temp+2],0
	mov byte [temp+3],0
	push dword [temp]
 	push BCDformLZ
	push dword [stderr]
 	CALL fprintf
 	add esp, 4*3 									;no  -> print 2 digits and go back to loop
 	jmp %%print_loop

%%finish_printing:
	printl "" 										;go down 1 line after printing number					 						;reduce the value in stackSize after popping number
	IncCounter
	popfd
	popad
%%end:	 
%endmacro

extern exit
extern printf 
extern fprintf 
extern malloc 
extern free
extern fgets 
extern stderr 
extern stdin 
extern stdout 

BUF_SIZE equ 100
STACK_MAX equ 5

section .data

sform: DB "%s",0

inputdebug: DB "Input: %s",0

stackdebug: DB "Result: ",0

nform: DB "%d",10,0

hxform: DB "0x%x",10,0

BCDformLZ: DB "%02x",0				;for printing leading zero (not first byte)	in 2 digits of BCD format

BCDformNZ: DB "%x",0				;for printing 2 BCD digits WITHOUT leading zero 
	
counter: DD 0

stackSize: DD 0

temp: DD 0

result:	DD 0

debug: DB 0

section .bss
input: RESB BUF_SIZE

stk: RESB STACK_MAX*4

section .text 
     align 16 
     global main

main:
	push ebp
	mov ebp, esp
	mov ecx, dword [ebp+8]			; this is argc
	cmp ecx, 2
	jl .no_debug
	mov ecx, dword [ebp+12]
	mov esi, dword [ecx + 4]
	cmp byte [esi], '-'
	jne .no_debug
	cmp byte [esi+1], 'd'
	jne .no_debug
	cmp byte [esi+2], 0
	jne .no_debug
	mov byte [debug], 1
	printl "Debugging mode"
.no_debug:
	CALL my_calc	
	printn eax
	push 0
	mov esp, ebp
	pop ebp
	call exit

my_calc:
	push ebp
	mov ebp, esp
	pushad
my_calc_loop:
	print ">>calc: "
	push dword [stdin]
	push BUF_SIZE
	push input
	CALL fgets				;get line from user
	add esp, 4*3
	cmp byte [debug], 0
	je .nodebug
	pushad
	push input
	push inputdebug
	push dword [stderr]
	call fprintf
	add esp, 4*3
	popad
.nodebug:
	jmp_if_operand input, 'q', .end 			; quit function
	jmp_if_operand input, '+', .addition 		; 
	jmp_if_operand input, 'p', .pop_and_print
	jmp_if_operand input, 'd', .duplicate
	jmp_if_operand input, 'r', .shift_right
	jmp_if_operand input, 'l', .shift_left

;check if number:
	mov ebx, 1 							;counter for total digits (at least 1)
	mov ecx, 0 							;counter for leading zeros
.check_leading_zeros:
	cmp byte [input + ebx - 1], '0' 	;check if current character is 0
	jne .check_digit 					;no  -> check this character and remaining as usual
	inc ecx 							;yes -> increase leading zeros counter and digits counter
	cmp byte [input + ebx], 10 			;check if input is only zeros
	je .addZero 						;yes -> add 1 zero to stack
	inc ebx
	jmp .check_leading_zeros 			;no  -> check if next digit is zero


.check_digit:
	cmp byte [input + ebx - 1], '9'
	jg .illegal
	cmp byte [input + ebx - 1], '0'
	jl .illegal
	cmp byte [input + ebx], 10
	jle .number 					
	inc ebx								;ebx = num of digits
	jmp .check_digit

.number:
	mov eax, [stackSize]
	cmp eax, STACK_MAX - 1
	jle .number2 						;at this point the number is valid and we can add it to the stack
	printl ">>Error: Operand Stack Overflow"
	jmp my_calc_loop
.number2:
	new_node							;eax holds the address of allocated memory
	mov esi, eax						;esi holds the pointer to the first node of the number to be put in the stack
	mov edx, eax 						;edx holds the pointer to the next node
	add ecx, 2
	cmp ebx, ecx ;ecx=leading zeros + 2	;check if (num of digits >= number of leading zeros + 2)
	jge .PREeven						;yes -> at least 2 more digits are left from the number	
	sub ecx, 1

 
.addZero:
	mov eax, [stackSize]
	cmp eax, STACK_MAX - 1
	jle .addZero2 						;at this point the number is valid and we can add it to the stack
	printl ">>Error: Operand Stack Overflow"
	jmp my_calc_loop								;add 1 zero to stack if input is only zeros
.addZero2:	
	new_node	 						;malloc(5)
	mov esi, eax 						;pointer to only element in number (0)
	mov edx, eax 						;pointer to the location of link that will be changed

;apparently the sub operand affects the flag checked by conditional jumps, so I changed it after the jmp
.PREodd: 								;else, only 1 digit left and its the last one
	sub ecx, 1
.odd:									
	mov al, byte [input + ebx - 1]		;put last digit character in ebx
	sub al, '0'							;substract the ascii value of 0 char to get int value of current digit		
	mov byte [edx], al					;put last digit in memory
	jmp .put_in_stack 					;last digit added, number will be put in stack

;apparently the sub operand affects the flag checked by conditional jumps, so I changed it after the jmp 
.PREeven:
sub ecx, 2
.even:
	mov al, byte [input + ebx - 1] 			;put last digit character in al
	dec ebx
	mov ah, byte [input + ebx - 1] 			;put the digit before the last in ah. ax holds 2 last digits
	dec ebx
	sub al, '0' 							;change ascii value to int value of number
	sub ah, '0'								;""                                     ""	
	shl ah, 4 								;shift higher digit to higher part of byte
	add al, ah 								;create the 2 digit BCD number
	mov byte [edx], al 						;put number in first byte of node
	cmp ebx, ecx							;check if no more digits are left besides leading zeros
	je .put_in_stack 						;yes -> the number will be put in the stack
	new_node 								;no  -> allocate memory for next node
	mov dword [edx + 1], eax 				;put in the 4 bytes of current node the address of the next node
	mov edx, eax 							;put address of allocated memory of the next node in edx
	inc ecx
	cmp ebx, ecx ;ecx = leading zeros + 1 	;check if 1 digit is left to add besides leading zeros
	je .PREodd 								;yes -> jump to adding 1 digit
	dec ecx
	jmp .even 								;no  -> more than 1 digit left, jump to adding 2 digits
.put_in_stack:
	mov dword [edx + 1], 0 				    ;put null in address of last node
	mov edi, [stackSize] 					;put current open location of stack in edi
	mov dword [stk + edi*4], esi 			;put number in the current available place in stack
	inc edi
	mov dword [stackSize], edi 				;increase stackSize value by 1
	jmp my_calc_loop

.illegal:
	printl ">>Error: Illegal Input"
	jmp my_calc_loop


.pop_and_print:
	mov eax, [stackSize]
	cmp al, 1 														 ;check if stack is not empty
	jge .pop_and_print2 										   	 ;yes -> continue
	printl ">>Error: Insufficient Number of Arguments on Stack" 	 ;no  -> print args error and return to start
	jmp my_calc_loop

.pop_and_print2:
	mov bl, '~' 									
	push ebx 										;put ~ in stack to indicate end of printing
	mov ecx, dword [stackSize] 						;put current location of stack in ecx	
	mov edx, dword [stk + (ecx-1)*4] 				;put address of number in the current location of stack in edx
	mov esi, edx
;numbers are pushed to stack from low digit to high
.pushing_digits_loop:								;loop of adding digits to stack
	mov bl, byte [edx] 								;put number value of current node in bl
	push ebx										;put number in stack
	cmp dword [edx + 1], 0 							;check if address of next node is 0
	je .start_printing 								;yes -> start printing digits from stack
	mov edx, [edx + 1]								;no  -> get address of next node and return to beginning of loop
	jmp .pushing_digits_loop

.start_printing:
	print ">>"
 	pop ebx 										;pop register holding last digit/s
 	printBCDnz bl 									;print 2 digits, if 1st digit is zero it won't be printed
.print_loop:
 	pop ebx 										;pop next 2 digits/finish flag
 	cmp bl, '~' 									;check if popped element is finish flag
 	je .finish_printing 							;yes -> finish printing
 	printBCDlz bl 									;no  -> print 2 digits and go back to loop
 	jmp .print_loop

.finish_printing:
	printl "" 										;go down 1 line after printing number
	push esi
	call free_mem
	add esp, 4
	dec ecx
	mov dword [stackSize], ecx 						;reduce the value in stackSize after popping number
	IncCounter
	jmp my_calc_loop 									;return to start


.duplicate:
	mov ebx, [stackSize]
	cmp ebx, 1 															;check if stack is not empty
	jge .duplicate2  													;yes -> continue
	print ">>Error: Insufficient Number of Arguments on Stack"			;no  -> print error and return to start
	jmp my_calc_loop
.duplicate2:
	cmp ebx, STACK_MAX - 1 					;check if stack is not full
	jle .duplicate3 						;yes -> start
	print ">>Error: Operand Stack Overflow"	;no  -> print error and return to start
	jmp my_calc_loop
.duplicate3:
	mov ecx, dword [stk + (ebx-1)*4]		;ecx <- pointer to last element in stackSize
	inc ebx
	mov dword [stackSize], ebx 				;stackSize <- stacksize + 1
	new_node 								;create first 
	mov dword [stk + (ebx-1)*4], eax 		;put pointer to first link in top of stack
	mov edx, eax 							;edx will be used instead of eax for new links
.duplicate_loop:	
	mov al, byte [ecx]						;al <- old link.value		
	mov byte [edx], al 						;new link.value <- old link.value
	cmp dword [ecx+1], 0 					;check if old link.next is null
	je .finish_duplicate 					;yes -> finish duplicating
	mov ecx, dword [ecx+1] 					;no  -> old link <- old link.next
	new_node
	mov dword [edx+1], eax 					;new link.next <- new malloced link
	mov edx, dword [edx+1] 					;new link <- new link.next
	jmp .duplicate_loop 					;duplicate next link
.finish_duplicate:
	mov dword [edx+1], 0 					;last new link.next <- null
	debugTop
	IncCounter	
	jmp my_calc_loop

.shift_right:
	mov esi, [stackSize]
	cmp esi, 2 														 ;check if stack is not empty
	jge .shift_right2 											   	 ;yes -> continue
	printl ">>Error: Insufficient Number of Arguments on Stack" 	 ;no  -> print args error and return to start
	jmp my_calc_loop
.shift_right2:
	mov edi, dword [stk+(esi-1)*4] 			;edi <- pointer to k value
	cmp dword [edi+1], 0 					;check if k value is lesser than 99
	je .shift_right3 						;no  -> continue shift
	printl ">>Error: exponent too large" 	;yes -> print error and return to start
	jmp my_calc_loop
.shift_right3:	
	dec esi 								;decrease stackSize
	mov dword [stackSize], esi 				;stackSize <- new stackSize	
	mov cl, byte [edi] 						;cl <- k.val in BCD
	and cl, 15  							;cl <- lower digit of k.val
	mov al, byte [edi] 						;al <- k.val in BCD
	shr al, 4 								;al <- higher digit of k.val
	mov bl, 10 								
	mul bl									;al <- al*10
	add cl, al 								;cl <- higher digit * 10 + lower digit
	cmp cl, 0 								;check if k == 0
	je .shift_end 							;yes -> return x
.right_loop:
	mov edi, dword [stk+(esi-1)*4] 			;edi <- pointer to x value
	mov al, byte [edi]
	and al, 15
	shr al, 1
	mov bl, byte [edi]
	shr bl, 5
	jnc .no_remainder
	add al, 5
.no_remainder:
	shl bl, 4
	add bl, al
	mov byte [edi], bl
	cmp dword [edi+1], 0
	jne .inner_right_loop
	dec cl
	jnz .right_loop
	jmp .shift_end
.inner_right_loop:
	mov edx, dword [edi+1] 
	mov al, byte [edx]
	and al, 15
	shr al, 1
	jnc .no_remainder2
	add bl, 5*16
	mov byte [edi], bl
.no_remainder2:	
	mov bl, byte [edx]
	shr bl, 5
	jnc .no_remainder3
	add al, 5
.no_remainder3:
	shl bl, 4
	add bl, al
	mov byte [edx], bl
	cmp dword [edx+1], 0
	jne .next_digits
	cmp byte [edx], 0
	jne .positive_last_link
	mov dword [edi+1], 0
.positive_last_link:	
	dec cl
	jnz .right_loop
	jmp .shift_end
.next_digits:	
	mov edi, edx
	mov edx, dword [edx+1]
	jmp .inner_right_loop
.shift_left:
	mov esi, [stackSize]
	cmp esi, 2 														 ;check if stack is not empty
	jge .shift_left2 											   	 ;yes -> continue
	printl ">>Error: Insufficient Number of Arguments on Stack" 	 ;no  -> print args error and return to start
	jmp my_calc_loop
.shift_left2:
	mov edx, dword [stk+(esi-1)*4] 			;edx <- pointer to k value
	cmp dword [edx+1], 0 					;check if k value is lesser than 99
	je .shift_left3 						;no  -> continue shift
	printl ">>Error: exponent too large" 	;yes -> print error and return to start
	jmp my_calc_loop
.shift_left3:	
	dec esi 								;decrease stackSize
	mov dword [stackSize], esi 				;stackSize <- new stackSize
	mov cl, byte [edx] 						;cl <- k.val in BCD
	and cl, 15  							;cl <- lower digit of k.val
	mov al, byte [edx] 						;al <- k.val in BCD
	push edx
	call free_mem
	add esp, 4
	shr al, 4 								;al <- higher digit of k.val
	mov bl, 10 								
	mul bl									;al <- al*10
	add cl, al 								;cl <- higher digit * 10 + lower digit
	cmp cl, 0 								;check if k == 0
	je .shift_end 							;yes -> return x
	mov ebx, dword [stk+(esi-1)*4] 			;ebx <- pointer to x value
.left_loop:
	push ebx 								;put pointer to x in stack
	push ebx 								;put pointer to x in stack
	CALL addition2 							;eax <- x*2
	push ebx
	call free_mem
	add esp, 4
	mov ebx, eax 							;ebx <- eax
	add esp, 8
	loop .left_loop 						;k <- k - 1 ; return to loop
	mov dword [stk+(esi-1)*4], ebx 			;when k == 0: top of stack <- pointer to result
.shift_end:
	debugTop
	IncCounter 								;operands counter ++
	jmp my_calc_loop

.end:
	popad
	mov eax, [counter]		; move num of legal operations made to eax
	leave
	ret

.addition:
	mov ebx, dword [stackSize]	
	cmp ebx, 2 								;check if there are at least 2 elements in stack
	jl .error_args 							;no -> not enough arguments for addition
	push dword [stk + (ebx-1)*4] 			;push 2nd number to function args
	push dword [stk + (ebx-2)*4] 			;push 1st number to function args
	CALL addition2 							;eax <- 1st arg + 2nd arg
	call free_mem
	add esp, 4
	call free_mem
	add esp, 4
	dec ebx 							 	;ebx holds the new stack size after addition
	mov [stackSize], ebx 					;stackSize <- stackSize - 1
	mov dword [stk + (ebx-1)*4], eax 		;put result in top of stack
	debugTop
	IncCounter 								;operands counter ++
	jmp my_calc_loop						

.error_args:
	printl ">>Error: Insufficient Number of Arguments on Stack" ;no  -> print error and continue
	jmp my_calc_loop
	
addition2:
	push ebp
	mov ebp, esp
	pushad
	mov esi, dword [ebp + 8] 				;1st arg of addition at esi
	mov edi, dword [ebp + 12] 				;2nd arg of addition at edi									
	new_node
	mov dword [result], eax 				;put pointer to number in result
	mov ebx, eax 							;ebx now holds pointer to new allocated number
.while:
	mov al, byte [esi] 						;al = esi.value (located at first byte)
	adc al, byte [edi] 						;al = al + edi.value + CF						
	DAA 									;adjust the sum of al+dl+CF to BCD format in al, and put 1 in CF if overflowed
	mov byte [ebx], al 						;put result in result.value
	pushfd 									;push EFLAGS to stack to avoid being affected by cmp instruction
	cmp dword [esi+1], 0 					;check if esi.next == null
	je .esi0 								
	cmp dword [edi+1], 0 					;check if edi.next == null
	je .esi1_edi0
.esi1_edi1:	 								;(esi.next != null & edi.next != null)
	new_node
	mov dword [ebx+1], eax 					;ebx.next <- new allocated link
	mov ebx, eax							;ebx <- ebx.next
	mov esi, dword [esi+1] 					;esi <- esi.next (esi.next != null)
	mov edi, dword [edi+1] 					;edi <- esi.next (esi.next != null)
	popfd 									;restore EFLAGS
	jmp .while

.esi0: 										;esi.next == null
	cmp dword [edi+1], 0 					;check if edi.next == null
	je .end
.esi0_edi1:
	mov esi, dword [edi+1]					; esi now holds pointer to the *only* number list left
	jmp .single_arg
.esi1_edi0: 								;(esi != null & edi == null)
	mov esi, dword [esi+1] 					;esi <- esi.next (esi.next != null)
.single_arg:
	popfd 									;restore EFLAGS
	new_node
	mov dword [ebx+1], eax 					;ebx.next <- new link
	mov ebx, eax 							;ebx <- ebx.next
	mov al, 0
	adc al, byte [esi] 						;al <- esi.value + CF
	DAA 									;adjust to BCD
	mov byte [ebx], al 						;put value in result.val
	pushfd 									;store EFLAGS in stack
	cmp dword[esi+1], 0 					;check if esi.next == null
	je .end 								;yes -> finish
	mov esi, dword [esi+1] 					;no  -> esi <- esi.next
	jmp .single_arg
.end:
	popfd 									;restore EFLAGS 
	jnc .end2 								;if CF=0, the addition has ended
	new_node 								;else, create a new link with the carried value 1
	mov dword [ebx+1], eax
	mov ebx, eax
	mov byte [ebx], 1
.end2:
	mov dword [ebx+1], 0 					;last link points to null
	popad
	mov eax, dword [result] 				;put result pointer in eax return value
	mov esp, ebp
	pop ebp
	ret

free_mem:
	push ebp
	mov ebp, esp
	pushad
	pushfd
	mov ebx, [ebp + 8]
	cmp dword [ebx+1], 0
	jne .recursive
	jmp .finish
.recursive:
	push dword [ebx+1]
	call free_mem
	add esp, 4
.finish:
	push ebx
	CALL free
	add esp, 4
	popfd
	popad
	mov esp, ebp
	pop ebp
	ret