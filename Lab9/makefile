fileName = task3
utilName =
asmName =

ExecCFile = gcc -m32 -g -Wall -o
CFile = gcc -m32 -g -ansi -Wall -c -o
SFile = nasm -g -f elf -w+all -o
ExecSFile = ld -m elf_i386 -o

all: $(fileName)

#create execution file
$(fileName): $(fileName).o #$(utilName).o $(utilName).h
	$(ExecSFile) $(fileName) $(fileName).o #$(utilName).o

#compile main file
$(fileName).o: $(fileName).s
	$(SFile) $(fileName).o $(fileName).s

#compile utility file
#$(utilName).o: $(utilName).c
#	$(CFile)  $(utilName).o $(utilName).c

# compile assembly file
#$(asmName).o: $(asmName).s
#	$(SFile) $(asmName).s $(asmName).o

.PHONY: clean

clean:
	rm -f *.o $(fileName)