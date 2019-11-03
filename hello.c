#include "system.h"

int main(int argc, char** argv) {
	int top = 1000, bottom = -1000, ystep = 50;
	int left = -2500, right = 1000, xstep = 30;
	int maxiter = 1000;

	for (int y0 = top; y0 > bottom; y0 -= ystep) {
		for (int x0 = left; x0 < right; x0 += xstep) {
			int i = 0, x = 0, y = 0, ch = ' ';
			while (i < maxiter) {
				int x2 = x * x / 1000;
				int y2 = y * y / 1000;
				if ((x2 + y2) > 4000) {
					ch = (i > 9) ? '@' : (i + '0');
					break;
				}
				y = 2 * x * y / 1000 + y0;
				x = x2 - y2 + x0;
				i++;
			}
			dputc(ch);
		}
		dputc('\n');
	}
	return 0;
}
