
extern unsigned char state[];
extern unsigned char WorldLength, WorldWidth;

unsigned char cell(int x,int y) {
	int counter = 0, res = 0, a = 0, b = 0, i = 0, j = 0;
	for (i = 0 ; i < 3 ; i++)
		for (j = 0 ; j < 3 ; j++) {
			a = (x-1+j+WorldWidth)%WorldWidth;
			b = (y-1+i+WorldLength)%WorldLength;
			if (a != x || b != y) {
				if (state[b*WorldWidth+a] != '0')
					counter++;
			}
		}
	if (counter == 3) {	
		res = (state[y*WorldWidth + x] - '0');
		if (res < 9)
			res++;
	}
	else {
		if (counter == 2) {
			if (state[y*WorldWidth + x] != '0') {
				res = (state[y*WorldWidth + x] - '0');
				if (res < 9)
					res++;
			}
		}
	}
	return res;
}