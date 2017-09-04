#include <stdio.h>

int digit_cnt(char* str) {
	int counter = 0, i = 0;
	while (str[i] != 0) {
		if (str[i] >= '0' && str[i] <= '9')
			counter++;
		i++;
	}
	return counter;
}

int main(int argc, char *argv[]) {
	if (argc == 1)
		printf("usage: ./ntsc <string>\n");
	else
		printf("The number of digits in the string is: %d\n", digit_cnt(argv[1]));
	return 0;
}