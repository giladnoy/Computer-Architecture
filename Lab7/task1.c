#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define BUF_SIZE 102

typedef struct{
  	char *name;
  	char (*fun)(void);
} menu_struct;

int debug = 0, size = 1, options_num = 0;
char *filename = NULL;
char *data_pointer = NULL;

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

void setName() {
	free(filename);
	filename = (char*) malloc(BUF_SIZE);
	printf("Enter new file name: ");
	fgets(filename, BUF_SIZE, stdin);
	filename = strtok(filename, "\n");
	if (debug)
		printf("Debug: file name set to %s\n",filename);
}

void setSize() {
	int c;
	printf("Enter new unit size: ");
	c = getchar() - '0';
	while (getc(stdin) != '\n'); /*flush the input*/
	if (c == 1 || c == 2 || c == 4) {
		size = c;
		if (debug)
			printf("Debug: set size to %d\n", size);
	}
	else
		printf("Error: %d is not a valid unit size\n", c);
}

void fDisplay() {
	FILE *f = NULL;
	unsigned int location = 0, length = 0, i = 0, j = 0, sum = 0;
	unsigned char *buffer;
	char buffer2[BUF_SIZE];
	if (filename == NULL) {
		printf("Error: No file chosen, please set file name\n");
		return;
	}
	f = fopen(filename, "r");
	if (f == NULL) {
		perror("Error");
		return;
	}
	printf("Please enter <location> <length>\n");
	fgets(buffer2, BUF_SIZE, stdin);
	location = (unsigned int) strtol(strtok(buffer2," "), NULL, 16); /*convert hex string to int value*/
	length = atoi(strtok(NULL,"\n")); /*strtok points to next position of buffer2, so NULL is passed as 1st arg*/
	if (debug) {
		printf("Debug: location: 0x%x, length: %d\n",location, length);
	}
	buffer = (unsigned char*) malloc(size*length);
	fseek(f, location, SEEK_SET);
	fread(buffer,size, length, f);
	fclose(f);
	printf("Hexadecimal Representation:\n");
	for (i = 0 ; i < length ; i++) {
		for (j = 0 ; j < size ; j++) {
			sum += buffer[i*size+j]*(unsigned int)(pow(256,j));
		}
		printf("%04x",sum);
		sum = 0;
		if (i < length - 1)
			printf(" ");
	}
	printf("\nDecimal Representation:\n");
	for (i = 0 ; i < length ; i++) {
		for (j = 0 ; j < size ; j++) {
			sum += buffer[i*size+j]*(unsigned int)(pow(256,j));
		}
		printf("%d",sum);
		sum = 0;
		if (i < length - 1)
			printf(" ");
	}
	printf("\n");
	free(buffer);
}

void loadmem() {
	FILE *f = NULL;
	unsigned char buffer[BUF_SIZE];
	unsigned int location = 0, length = 0, ad = 0;
	if (filename == NULL) {
		printf("Error: No file chosen, please set file name\n");
		return;
	}
	f = fopen(filename, "r");
	if (f == NULL) {
		perror("Error");
		return;
	}
	printf("Please enter <mem-address> <location> <length>\n");
	fgets(buffer, BUF_SIZE, stdin);
	ad = (unsigned int) strtol(strtok(buffer," "), NULL, 16);
	location = (unsigned int) strtol(strtok(NULL," "), NULL, 16); /*convert hex string to int value*/
	length = strtol((strtok(NULL,"\n")), NULL, 10); /*strtok points to next position of buffer, so NULL is passed as 1st arg*/
	free(data_pointer);
	if (ad)
		data_pointer = ad;
	else 
		data_pointer = (char*) malloc(length);
	if (debug)
		printf("Debug: File name: %s, data_pointer address: 0x%x, location: 0x%x, length: %d\n",filename, data_pointer ,location, length);
	fseek(f, location, SEEK_SET);
	fread(data_pointer,1, length, f);
	fclose(f);
	printf("Loaded %d bytes into 0x%x\n",length,data_pointer);
}

void save() {
	FILE *f = NULL;
	unsigned char buffer[BUF_SIZE];
	unsigned char *buffer2;
	unsigned int location = 0, length = 0, ad = 0, endfile = 0;
	if (filename == NULL) {
		printf("Error: No file chosen, please set file name\n");
		return;
	}
	f = fopen(filename, "r+");
	if (f == NULL) {
		perror("Error");
		return;
	}
	printf("Please enter <source-address> <target-location> <length>\n");
	fgets(buffer, BUF_SIZE, stdin);
	ad = (unsigned int) strtol(strtok(buffer," "), NULL, 16);
	if (!ad) {
		if (data_pointer == NULL) {
			printf("Error: source address at the data pointer does not have a valid value\n");
			return;
		}
		ad = data_pointer;
	}
	location = (unsigned int) strtol(strtok(NULL," "), NULL, 16); /*convert hex string to int value*/
	length = atoi(strtok(NULL,"\n")); /*strtok points to next position of buffer, so NULL is passed as 1st arg*/
	if (debug)
		printf("Debug: File name: %s, source address: 0x%x, target location: 0x%x, length: %d\n",filename, ad ,location, length);
	fseek(f, 0, SEEK_END);
	endfile = ftell(f);
	if (location > endfile) {
		printf("Error: target location: 0x%x exceeds the end of the file 0x%x\n", location, endfile);
		return;
	}
	fseek(f, location, SEEK_SET);
	buffer2 = (unsigned char*) malloc(length+1);
	memcpy(buffer2, ad, length);
	fwrite(buffer2, 1, length, f);
	free (buffer2);
	fclose(f);
}

void modify() {
	FILE *f = NULL;
	char buffer[BUF_SIZE];
	unsigned char c[1];
	unsigned int location = 0, val = 0, endfile = 0, temp = 0, j = 0;
	if (filename == NULL) {
		printf("Error: No file chosen, please set file name\n");
		return;
	}
	f = fopen(filename, "r+");
	if (f == NULL) {
		perror("Error");
		return;
	}
	printf("Please enter <location> <val>\n");
	fgets(buffer, BUF_SIZE, stdin);
	location = (unsigned int) strtol(strtok(buffer," "), NULL, 16);
	val = (unsigned int) strtol(strtok(NULL,"\n"), NULL, 16);
	if (debug)
		printf("Debug: location: 0x%x, val: 0x%x\n", location, val);
	if (val < pow(256,size-1) || val > pow(256,size) - 1) {
		printf("val should be between 0x%x and 0x%x, but its value is 0x%x\n",(unsigned int)(pow(256,size-1)),(unsigned int)(pow(256,size)-1),val);
		return;
	}
	fseek(f, 0, SEEK_END);
	endfile = ftell(f);
	if (location > endfile) {
		printf("Error: target location: 0x%x exceeds the end of the file 0x%x\n", location, endfile);
		return;
	}
	fseek(f,location,SEEK_SET);
	for (j = 0 ; j < size ; j++) {
		temp = val / pow(256,size-j-1);
		c[0] = temp;
		fwrite(c,1,1,f);
	}
	fclose(f);
}

void quit() {
	if (debug) {
		printf("Debug: quitting\n");
	}
	exit(0);
}

int inbound(int i) {
	if (i >= 0 && i < options_num)
		return 1;
	return 0;
}

int main(int argc, char *argv[]) {
	int i;
	int c;
	menu_struct options[] = {{"Toggle Debug Mode",toggleDebug},{"Set File Name",setName},
	{"Set Unit Size",setSize},{"File Display", fDisplay}, {"Load Into Memory",loadmem},
	{"Save Into File",save},{"File Modify",modify},{"Quit",quit},{NULL,NULL}};
	while (options[++options_num].name != NULL); /*set the number of options variable */
	while (1) {
		if (debug) {
			printf("Debug: Unit size: %d, File name: %s, Buffer address: 0x%x\n",size,filename,data_pointer);
		}
		printf("Choose action: \n");
		i = 0;
		while (options[i].name != NULL) {
			printf("%d)-%s\n",i,options[i].name);
			i++;
		}
		c = (getchar() - '0'); /*get the int value from the char value*/
		while (getc(stdin) != '\n'); /*flush input*/
		if (inbound(c)) {
			options[c].fun();
		}
		else
			printf("Option is not within bounds\n");
	}
}