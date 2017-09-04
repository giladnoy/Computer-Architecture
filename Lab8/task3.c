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

Elf32_Shdr* getSectionHeader(int index) {
	return (Elf32_Shdr*)(map_start + header->e_shoff + index*header->e_shentsize);
}


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

  /*Elf32_Ehdr;*/
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

}

void printSections() {
	int i, address, offset, size;
	char *name;
	char *type;
	char *stringTable;
	char *sec_type[] = {"NULL","PROGBITS","SYMTAB","STRTAB","RELA","HASH","DYNAMIC","NOTE","NOBITS","REL","SHLIB","DYNSYM"};
	Elf32_Shdr *sHeader;
	if (Currentfd < 0) {
		fprintf(stderr,"Error: no valid ELF file selected\n");
		return;
	}
	sHeader = getSectionHeader(header->e_shstrndx); /*(Elf32_Shdr*) (map_start + secOffset + (strIdx*entSize));*/ /*sHeader points to the section header string table entry*/
	if (debug) {
		fprintf(stderr,"Debug section string table: idx:%d, offset in file:%x\n",header->e_shstrndx,sHeader->sh_offset);
	}
	stringTable = (char*) (map_start + sHeader->sh_offset);
	printf("idx  %-20s %-8s %-6s %-6s %-10s\n", "Name", "Address", "Offset", "Size", "Type");
	for (i = 0 ; i < header->e_shnum ; i ++) {
		sHeader = getSectionHeader(i); /*(Elf32_Shdr*) (map_start + header->e_shoff + (i*entSize));*/ /*Location of start of file + offset of section header table*/
		name = &stringTable[sHeader->sh_name];
		address = sHeader->sh_addr;
		offset = sHeader->sh_offset;
		size = sHeader->sh_size;
		type = (char*)((int)sHeader->sh_type<12?(char*)sec_type[sHeader->sh_type]:"OTHER");
		printf("[%02d] %-20s %08x %06x %06x %-10s\n",i,name,address,offset,size,type);
	}
	printf("\n");
}

void printSymbols() {
	int i, value, sec_idx, sym_num, sec_str_idx;
	char *sec_name, *sym_name, *secStrTable, *strTable;
	Elf32_Shdr *sHeader, *tmpHeader;
	Elf32_Sym *curSym;
	sHeader = getSectionHeader(header->e_shstrndx); /*sHeader points to the section header string table entry*/
	secStrTable = (char*) (map_start + sHeader->sh_offset);
	for (i = 0 ; i < header->e_shnum ; i ++) {
		sHeader = getSectionHeader(i);
		if (sHeader->sh_type == SHT_SYMTAB) /*Current section header is the symbol section*/
			break;

	}
	tmpHeader = getSectionHeader(sHeader->sh_link); /*the symbol section (sHeader) link points to the index of string table*/
	strTable = (char*) (map_start + tmpHeader->sh_offset);
	sym_num = sHeader->sh_size / sizeof(Elf32_Sym); /*size of symbol section / size of each symbol = num of symbols*/
	if (debug)
		fprintf(stderr, "Debug: Size of symbol table in bytes:%d, num of symbols:%d\n",sHeader->sh_size, sym_num);
	printf("SymIdx %-8s%3s %-20s %s\n", "Value", "SecIdx", "sec_name", "sym_name");
	for (i = 0 ; i < sym_num ; i++) {
		sym_name = "";
		if (debug)
			fprintf(stderr,"Debug: ");
		curSym = (Elf32_Sym*) (map_start + sHeader->sh_offset + i*sizeof(Elf32_Sym));
		value = curSym->st_value;
		if (debug)
			fprintf(stderr,"value:%x, ",value);
		sec_idx = curSym->st_shndx;
		if (debug)
			fprintf(stderr,"sec_idx:%d, ",sec_idx);
		sym_name = &strTable[curSym->st_name];
		if (debug)
			fprintf(stderr,"symbol name:%s ",sym_name);
		if (sec_idx == SHN_ABS || sec_idx == SHN_UNDEF) {
			sec_name = (sec_idx == SHN_ABS)? "ABS" : "UND";
			if (debug)
				fprintf(stderr,"sec_name: %s\n",sec_name);
			printf("[%02d]   %08x   %s %-20s %s\n",i,value,sec_name,"",sym_name);
		}
		else {
			tmpHeader = getSectionHeader(sec_idx);
			sec_str_idx = tmpHeader->sh_name;
			if (debug)
				fprintf(stderr,"sec_name_idx:%d, ",sec_str_idx);
			sec_name = &secStrTable[sec_str_idx];
			if (debug)
				fprintf(stderr,"sec_name_:%s, \n",sec_name);
			printf("[%02d]   %08x   %3d %-20s %s\n",i,value,sec_idx,sec_name,sym_name);
		}
	}
	printf("\n");
}

void linkCheck() {
	int sym_num,i;
	Elf32_Shdr *sHeader, *tmpHeader;
	Elf32_Sym *curSym;
	char *strTable;
	char *check = "FAILED";
	for (i = 0 ; i < header->e_shnum ; i ++) {
		sHeader = getSectionHeader(i);
		if (sHeader->sh_type == SHT_SYMTAB) /*Current section header is the symbol section*/
			break;
	}
	tmpHeader = getSectionHeader(sHeader->sh_link); /*the symbol section (sHeader) link points to the index of string table*/
	strTable = (char*) (map_start + tmpHeader->sh_offset);
	sym_num = sHeader->sh_size / sizeof(Elf32_Sym); /*size of symbol section / size of each symbol = num of symbols*/
	for (i = 0 ; i < sym_num ; i++) {
		curSym = (Elf32_Sym*) (map_start + sHeader->sh_offset + i*sizeof(Elf32_Sym));
		if (!strcmp(&strTable[curSym->st_name],"_start")) /*check if current symbol name is _start*/
			check = "PASSED";
	}
	printf("_start check: %s\n",check);
}


void rawRelocTable() {
	int i, j, entNum;
	Elf32_Shdr *sHeader;
	Elf32_Rela *rela;
	for (i = 0 ; i < header->e_shnum ; i++) {
		sHeader = getSectionHeader(i);
		/*if (sHeader->sh_type == SHT_REL) {
			entNum = (sHeader->sh_size / sHeader->sh_entsize);
			for (j = 0 ; j < entNum ; j++) {
				rel = (Elf32_Rel*) (map_start + sHeader->sh_offset + j*sHeader->sh_entsize);
				printf("r_offset: %x, r_info: %x\n",rel->r_offset,rel->r_info);
			}
		}*/
		if (sHeader->sh_type == SHT_REL) {
			entNum = (sHeader->sh_size / sHeader->sh_entsize);
			for (j = 0 ; j < entNum ; j++) {
				rela = (Elf32_Rela*) (map_start + sHeader->sh_offset + j*sHeader->sh_entsize);
				printf("r_offset: %x, r_info: %x, r_addend: %x\n",rela->r_offset,rela->r_info, rela->r_addend);
			}
		}
	}
	printf("\n");
}

  /*Elf32_Shdr*/
  /*Elf32_Word	sh_name;		 Section name (string tbl index) */
  /*Elf32_Word	sh_type;		 Section type */
  /*Elf32_Word	sh_flags;		 Section flags */
  /*Elf32_Addr	sh_addr;		 Section virtual addr at execution */
  /*Elf32_Off	sh_offset;		 Section file offset */
  /*Elf32_Word	sh_size;		 Section size in bytes */
  /*Elf32_Word	sh_link;		 Link to another section */
  /*Elf32_Word	sh_info;		 Additional section information */
  /*Elf32_Word	sh_addralign;	 Section alignment */
  /*Elf32_Word	sh_entsize;		 Entry size if section holds table */

/*Elf32_Rel
  Elf32_Addr	r_offset;		Address 
  Elf32_Word	r_info;			Relocation type and symbol index */

/*Elf32_Rela
  Elf32_Addr	r_offset;		Address 
  Elf32_Word	r_info;			Relocation type and symbol index
  Elf32_Sword	r_addend;		Addend */

/*Elf32_Sym*/
  /*Elf32_Word	    st_name;		 Symbol name (string tbl index) */
  /*Elf32_Addr	    st_value;		 Symbol value */
  /*Elf32_Word	    st_size;		 Symbol size */
  /*unsigned char	st_info;		 Symbol type and binding */
  /*unsigned char	st_other;		 Symbol visibility */
  /*Elf32_Section	st_shndx;		 Section index */

/*#define ELF32_R_SYM(info)             ((info)>>8)

#define ELF32_R_INFO(sym, type)       (((sym)<<8)+(unsigned char)(type))*/

/*R_386_NONE 0 none none
R_386_32 1 word32 S + A
R_386_PC32 2 word32 S + A - P
R_386_GOT32 3 word32 G + A - P
R_386_PLT32 4 word32 L + A - P
R_386_COPY 5 none none
R_386_GLOB_DAT 6 word32 S
R_386_JMP_SLOT 7 word32 S
R_386_RELATIVE 8 word32 B + A
R_386_GOTOFF 9 word32 S + A - GOT
R_386_GOTPC 10 word32 GOT + A - P _ __________________________________________________ */

void semRelocTable() {
	int i, j, entNum, symIndex, relocType;
	Elf32_Shdr *dynsym=NULL, *sHeader=NULL, *symTable, *tmpHeader;
	Elf32_Rela *rela;
	Elf32_Sym *sym;
	char *dynStrTable, *secStrTable, *symStrTable, *sym_name="";
	char *relocTypes[] = {"R_386_NONE","R_386_32","R_386_PC32","R_386_GOT32","R_386_PLT32","R_386_COPY","R_386_GLOB_DAT","R_386_JMP_SLOT","R_386_RELATIVE","R_386_GOTOFF",};
	sHeader = getSectionHeader(header->e_shstrndx);
	secStrTable = (char*)(map_start + sHeader->sh_offset);
	for (i = 0 ; i < header->e_shnum ; i ++) {
		sHeader = getSectionHeader(i);
		if (sHeader->sh_type == SHT_SYMTAB){ /*Current section header is the dynamic symbol section*/
			symTable = sHeader;
			break;
		}
	}
	if (debug)
		fprintf(stderr,"Debug: Symbol table index:%d, type:%d\n",i,symTable->sh_type);
	sHeader = getSectionHeader(symTable->sh_link); /*sHeader points to the symbol string table section header*/
	symStrTable = (char*) (map_start + sHeader->sh_offset);
	for (i = 0 ; i < header->e_shnum ; i ++) {
		sHeader = getSectionHeader(i);
		if (sHeader->sh_type == SHT_DYNSYM){ /*Current section header is the dynamic symbol section*/
			dynsym = sHeader;
			break;
		}
	}
	if (dynsym) { /*There is a dynamic symbol table in the file*/
		sHeader = getSectionHeader(dynsym->sh_link); /*sHeader points to the dyn symbols string table*/
		dynStrTable = (char*) (map_start + sHeader->sh_offset);
	}
	for (i = 0 ; i < header->e_shnum ; i++) {
		sHeader = getSectionHeader(i);
		if (sHeader->sh_type == SHT_REL || sHeader->sh_type == SHT_RELA) {
			printf("Relocation section %s at offset 0x%x contains %d entries:\n",&secStrTable[sHeader->sh_name],sHeader->sh_offset,(sHeader->sh_size/sHeader->sh_entsize));
			printf("%-8s %-8s %-18s %-8s %s\n","Offset","Info","Types","Sym.Value","Sym.Name");
			entNum = (sHeader->sh_size / sHeader->sh_entsize);
			for (j = 0 ; j < entNum ; j++) {
				rela = (Elf32_Rela*) (map_start + sHeader->sh_offset + j*sHeader->sh_entsize);
				if (debug)
					fprintf(stderr,"Debug: r_offset:%x, r_info:%x, r_addend:%x\n",rela->r_offset,rela->r_info, rela->r_addend);
				symIndex = ELF32_R_SYM(rela->r_info);
				if (debug)
					fprintf(stderr,"Debug: symbol index of r_info: %x\n",symIndex);
				relocType = ELF32_R_TYPE(rela->r_info);
				if (debug)
					fprintf(stderr,"Debug: relocation type of r_info: %s\n",relocTypes[relocType]);
				if (dynsym) { /*Symbols appear in the dynamic symbol table*/	
					sym = (Elf32_Sym*) (map_start + dynsym->sh_offset + symIndex*dynsym->sh_entsize); /*get symbol from dynamic symbol table*/
					if (debug)
						fprintf(stderr,"Debug: symbol name:%s\n",&dynStrTable[sym->st_name]);
				
					printf("%08x %08x %-18s %08x %s\n",rela->r_offset,rela->r_info, relocTypes[relocType],sym->st_value, &dynStrTable[sym->st_name]);
				}
				else { /*symbols appear in the regular symbol table*/
					sym = (Elf32_Sym*) (map_start + symTable->sh_offset + symIndex*symTable->sh_entsize); /*get symbol from regular symbol table*/
					if (sym->st_name) { /*Name appears in the symbol string table*/
						if (debug)
							fprintf(stderr,"Debug: symbol name(if present):%s\n",&symStrTable[sym->st_name]);
						sym_name = &symStrTable[sym->st_name];
					}
					else { /*Name should be taken from the section header string table*/
						tmpHeader = getSectionHeader(sym->st_shndx);
						sym_name = &secStrTable[tmpHeader->sh_name];
					}
					printf("%08x %08x %-18s %08x %s\n",rela->r_offset,rela->r_info, relocTypes[relocType],sym->st_value, sym_name);
				}
			}
			printf("\n");
		}
	}
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
	{"Print Section Names",(void*)printSections},{"Print Symbols",(void*)printSymbols},{"Link check",(void*)linkCheck},
	{"Relocation Tables - Raw",(void*)rawRelocTable},{"Relocation Tables - Semantic",(void*)semRelocTable},
	{"Quit",(void*)quit},{NULL,NULL}};
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