#include <sys/ioctl.h>
#include <stdio.h>
#include <math.h>

#define TOO_MANY_COLOURS_IMPLEMENTATION
#include "too_many_colours.h"

int main(void) {
	struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
	if (w.ws_col == 0) w.ws_col = 20;
	if (w.ws_row == 0) w.ws_row = 20;

	for (int y = 0; y < w.ws_row; y++) {
		printf("\n");
		for (int x = 0; x < w.ws_col; x++) {
			//HSL hsl = HSL(((double)x) / ((double)w.ws_col) * 360.0, 1.0, 0.5);
			HSL hsl = HSL(270, ((double)x) / ((double)w.ws_col), ((double)y) / ((double)w.ws_row));
			RGB rgb = hsl_to_rgb(hsl);
			printf("\033[38;2;%d;%d;%dm", (int)round(255.0 * rgb.r), (int)round(255.0 * rgb.g), (int)round(255.0 * rgb.b));
			printf("\033[48;2;%d;%d;%dm", (int)round(255.0 * rgb.r), (int)round(255.0 * rgb.g), (int)round(255.0 * rgb.b));
			printf(" \033[0m");
		}
	}
	getc(stdin);
}
