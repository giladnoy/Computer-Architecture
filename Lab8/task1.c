#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MAX_BUFFER 100

typedef struct{
  	char *name;
  	char (*fun)(void);
} menu_struct;

void *map_start;

char buffer[MAX_BUFFER];
int debug = 0, options_num = 0;
int Currentfd = -1;
void *map_start = NULL; /* will point to the start of the memory mapped file */
struct stat fd_stat; /* this is needed to  the size of the file */
Elf32_Ehdr *header = NULL; /* this will point to the header structure */

void toggleDebug() {
	if (debug) {
		debug = 0;
		printf("Debug flag now off\n");
	}
	else {
		debug = 1;
		printf("Debug flag now on\n");
	}
}

void examine() {
	unsigned char *magicNum;
	printf("Enter ELF file name: ");
	char *scheme[] = {"No file type", "Relocatable file", "Executable file", "Shared object file", "Core file"};
	fgets(buffer,99,stdin);
	if (Currentfd > 0) {
		close(Currentfd);
	}
	if ((Currentfd = open(strtok(buffer,"\n"),O_RDONLY)) < 0) {
		perror("Open Error");
      	Currentfd = -1;
		return;
	}

	if(fstat(Currentfd, &fd_stat) != 0 ) {
      	perror("Stat Error");
      	close(Currentfd);
      	Currentfd = -1;
      	return;
    }
    if ((map_start = mmap(NULL, fd_stat.st_size, PROT_READ , MAP_SHARED, Currentfd, 0)) == MAP_FAILED ) {
    	perror("mmap Error");
     	close(Currentfd);
      	Currentfd = -1;
     	return;
    }
    header = (Elf32_Ehdr*) map_start;
    magicNum = header->e_ident;
    if (magicNum[1] != 'E' || magicNum[2] != 'L' || magicNum[3] != 'F') {
    	fprintf(stderr, "The selected file is not a valid ELF file");
    	munmap(header,fd_stat.st_size);
    	close(Currentfd);
    	Currentfd = -1;
    	header = NULL;
    	map_start = NULL;
    	return;
    }
    printf("Magic numbers:                       %c%c%c\n",magicNum[1],magicNum[2],magicNum[3]);
    printf("Data encoding scheme:                %s\n", header->e_type<5? scheme[header->e_type]:"Unknown");
    printf("Entry point:                         0x%08x\n",header->e_entry);
    printf("File offset of section header table: %d\n",header->e_shoff);
    printf("Number of section header entries:    %d\n",header->e_shnum);
    printf("Size of each section header entry:   %d\n",header->e_shentsize);
    printf("File offset of program header table: %d\n",header->e_phoff);
    printf("Number of program header entries:    %d\n",header->e_phnum);
    printf("Size of each program header entry:   %d\n\n",header->e_phentsize);

}/*}Elf32_Ehdr;*/
  /*unsigned char	e_ident[EI_NIDENT];	 Magic number and other info */
  /*Elf32_Half	e_type;			 Object file type */
  /*Elf32_Half	e_machine;		 Architecture */
  /*Elf32_Word	e_version;		 Object file version */
  /*Elf32_Addr	e_entry;		 Entry point virtual address */
  /*Elf32_Off	e_phoff;		 Program header table file offset */
  /*Elf32_Off	e_shoff;		 Section header table file offset */
  /*Elf32_Word	e_flags;		 Processor-specific flags */
  /*Elf32_Half	e_ehsize;		 ELF header size in bytes */
  /*Elf32_Half	e_phentsize;	 Program header table entry size */
  /*Elf32_Half	e_phnum;		 Program header table entry count */
  /*Elf32_Half	e_shentsize;	 Section header table entry size */
  /*Elf32_Half	e_shnum;		 Section header table entry count */
  /*Elf32_Half	e_shstrndx;		 Section header string table index */



/*{Elf32_Shdr;
  Elf32_Word	sh_name;		 Section name (string tbl index) */
  /*Elf32_Word	sh_type;		 Section type */
  /*Elf32_Word	sh_flags;		 Section flags */
  /*Elf32_Addr	sh_addr;		 Section virtual addr at execution */
  /*Elf32_Off	sh_offset;		 Section file offset */
  /*Elf32_Word	sh_size;		 Section size in bytes */
  /*Elf32_Word	sh_link;		 Link to another section */
  /*Elf32_Word	sh_info;		 Additional section information */
  /*Elf32_Word	sh_addralign;	 Section alignment */
  /*Elf32_Word	sh_entsize;		 Entry size if section holds table */

/*SHT_NULL 0
SHT_PROGBITS 1
SHT_SYMTAB 2
SHT_STRTAB 3
SHT_RELA 4
SHT_HASH 5
SHT_DYNAMIC 6
SHT_NOTE 7
SHT_NOBITS 8
SHT_REL 9
SHT_SHLIB 10
SHT_DYNSYM 11*/

void printSections() {
	int i, strIdx, entSize, secOffset, address, offset, size;
	char *name;
	char *type;
	char *stringTable;
	char *sec_type[] = {"NULL","PROGBITS","SYMTAB","STRTAB","RELA","HASH","DYNAMIC","NOTE","NOBITS","REL","SHLIB","DYNSYM"};
	Elf32_Shdr *sHeader;
	if (Currentfd < 0) {
		fprintf(stderr,"Error: no valid ELF file selected\n");
		return;
	}
	strIdx = header->e_shstrndx; /*index of string table section*/
	entSize = header->e_shentsize; /*size of each entry of section table*/
	secOffset = header->e_shoff; /*section table offset in file*/
	sHeader = (Elf32_Shdr*) (map_start + secOffset + (strIdx*entSize)); /*sHeader points to the section header string table entry*/
	if (debug) {
		fprintf(stderr,"Debug section string table: idx:%d, offset in file:%x\n",strIdx,sHeader->sh_offset);
	}
	stringTable = (char*) (map_start + sHeader->sh_offset);
	printf("idx  %-20s %-8s %-6s %-6s %-10s\n", "Name", "Address", "Offset", "Size", "Type");
	for (i = 0 ; i < header->e_shnum ; i ++) {
		sHeader = (Elf32_Shdr*) (map_start + header->e_shoff + (i*entSize)); /*Location of start of file + offset of section header table*/
		name = &stringTable[sHeader->sh_name];
		address = sHeader->sh_addr;
		offset = sHeader->sh_offset;
		size = sHeader->sh_size;
		type = (char*)((int)sHeader->sh_type<12?(char*)sec_type[sHeader->sh_type]:"OTHER");
		printf("[%02d] %-20s %08x %06x %06x %-10s\n",i,name,address,offset,size,type);
	}
	printf("\n");
}

void quit() {
	if (debug) {
		printf("Debug: quitting and unmapping memory\n");
	}
	if (header) {
		munmap(header,fd_stat.st_size);
		if (Currentfd>0)
			close(Currentfd);
	}
	exit(0);
}

int inbound(int i) {
	if (i >= 0 && i < options_num)
		return 1;
	return 0;
}

int main(int argc, char* argv[]) {
	int i = 0, c = 0;
	menu_struct options[] = {{"Toggle Debug Mode",(void*)toggleDebug}, {"Examine ELF File",(void*)examine},
	{"Print Section Names",(void*)printSections},{"Quit",(void*)quit},{NULL,NULL}};
	while (options[++options_num].name != NULL); /*set the number of options "options_num" variable */
	while (1) {
		if (debug) {
			fprintf(stderr,"Debug: fd:%d, mapping address:0x%08x \n",Currentfd, (unsigned int)header);
		}
		printf("Choose action: \n");
		i = 0;
		while (options[i].name != NULL) {
			printf("%d)-%s\n",i,options[i].name);
			i++;
		}
		c = (getchar() - '0'); /*get the int value from the char value*/
		while (getchar() != '\n'); /*flush input*/
		if (inbound(c)) {
			options[c].fun();
		}
		else
			printf("Option is not within bounds\n");
	}
}