fileName = ass3
asmName  = scheduler
asmName2 = printer
asmName3 = coroutines
utilName = cell

ExecCFile = gcc -m32 -Wall -g #-nostartfiles
CFile = gcc -m32 -g -ansi -Wall -c -o
SFile = nasm -f elf
ExecSFile = ld -m elf_i386

all: $(fileName)

#create execution file
$(fileName):  $(fileName).o $(asmName).o $(asmName2).o $(asmName3).o $(utilName).o
	$(ExecCFile) $(fileName).o $(asmName).o $(asmName2).o $(asmName3).o -o $(fileName) $(utilName).o

# compile assembly file
$(asmName).o: $(asmName).s
	$(SFile) $(asmName).s -o $(asmName).o

# compile assembly file 2
$(asmName2).o: $(asmName2).s
	$(SFile) $(asmName2).s -o $(asmName2).o

# compile assembly file 3
$(asmName3).o: $(asmName3).s
	$(SFile) $(asmName3).s -o $(asmName3).o	

#compile utility file
$(utilName).o: $(utilName).c
	$(CFile)  $(utilName).o $(utilName).c

#compile main file
$(fileName).o: $(fileName).s
	$(SFile) $(fileName).s -o $(fileName).o	

.PHONY: clean

clean:
	rm -f *.o $(fileName)