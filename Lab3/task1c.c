#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct virus virus;
 
struct virus {
    unsigned short length;
    char *name;
    char *signature;
}; 

typedef struct link link;
 
struct link {
    virus *v;
    link *next;
};

void printHex(char *buffer, int length) {
	int i;
	for (i = 0 ; i < length - 1; i++)
		printf("%02X ",(unsigned char) buffer[i]);
	printf("%02X\n",(unsigned char) buffer[i]);
}

int stringToInt(char *src, int endian){
	if (endian)
		return src[0]*256 + src[1];
	return src[0] + src[1]*256;
}

void printVirus(virus* vir) {
	int i;
	printf("Virus name: %s\n",vir->name);
	printf("Virus size: %d\n", vir->length);
	printf("signature: \n");
	printHex(vir->signature, vir->length);
	printf("\n");
}

void virus_free(virus *vir) {
	free(vir->name);
	free(vir->signature);
	free(vir);
}

void list_print(link *virus_list) {
	link *currLink = virus_list;
	virus *vir;
	while (currLink != NULL) {
		vir = currLink->v;
		printVirus(vir);
		currLink = currLink->next;
	}
}
 
link* list_append(link* virus_list, virus* data) {
	if (virus_list == NULL) {
		virus_list = (link*)malloc(sizeof(link));
		virus_list->v = data;
		virus_list->next = NULL;
		return virus_list;
	}
	link *currLink = virus_list;
	while (currLink->next != NULL)
		currLink = currLink->next;
	currLink->next = (link*)malloc(sizeof(link));
	currLink->next->v = data;
	currLink->next->next = NULL;
	return virus_list;
}
 
void list_free(link *virus_list) {
	if (virus_list != NULL) {
		list_free(virus_list->next);
		virus_free(virus_list->v);
		free(virus_list);
	}
}

void detect_virus(char *buffer, link *virus_list, unsigned int size) {
	if (virus_list != NULL) {
		int i = 0;
		virus *vir = virus_list->v;
		for (i = 0 ; i < size ; i++) {
			if (!(memcmp(&buffer[i],vir->signature,vir->length))) {
				printf("Virus starting location: %d\n",i);
				printf("Virus name: %s\n",vir->name);
				printf("Virus size: %d\n",vir->length);
			}
		}
		detect_virus(buffer, virus_list->next, size);
	}
}

int getMin(int a, int b) {
	if (a >= b) return a;
	return b;
}

int main(int argc, char **argv) {
	FILE *myFile = fopen("lab3_signatures_opt_2.txt","r");
	int endian = 1;
	int i = 0;
	char endianByte[1];
	char blockSize[2];
	char *block; 
	char *name;
	char *signature; 
	link *myList = NULL;
	virus *vir = NULL;
	int currPos = 0;
	int endPos = 0;
	int size = 0;
	fseek(myFile, 0, SEEK_END);
	endPos = ftell(myFile);
	rewind(myFile);
	currPos += fread(endianByte,sizeof(char),1,myFile); /*getting endian value (either 0 or 1)*/
	endian = endianByte[0];
	while (currPos < endPos) {

		currPos += fread(blockSize,sizeof(char),2,myFile); /*get the block size of current virus*/
		size = stringToInt(blockSize,endian);
		block = (char*)malloc(sizeof(char)*(size-2));
		currPos += fread(block,sizeof(char),size-2,myFile); /*getting name and signature of virus*/

		name = (char*)malloc(16*sizeof(char));
		for (i = 0; i < 16; i++)
			name[i] = block[i];

		signature = (char*)malloc((size - 18) * sizeof(char));
		for (i = 0; i < size - 18; i++)
			signature[i] = block[i + 16];

		vir = (virus*)malloc(sizeof(virus));
		*vir = (virus){.length = (size - 18), .name = name, .signature = signature};
		myList = list_append(myList,vir);
		free(block);
		block = NULL;
	}
	fclose(myFile);
	if (argc > 1) {
		FILE* suspect = fopen(argv[1],"r+");
		fseek(suspect, 0, SEEK_END);
		endPos = ftell(suspect);
		rewind(suspect);
		char buffer[10*1024];
		int minSize;
		minSize = getMin(sizeof(buffer)/sizeof(char), endPos);
		currPos = fread(buffer,sizeof(char),minSize,suspect);
		detect_virus(buffer,myList, minSize);
	}
	list_free(myList);
	return 0;
}
