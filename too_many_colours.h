#ifndef TOO_MANY_COLOURS_H_
#define TOO_MANY_COLOURS_H_

typedef struct {
	double r; /* red   [0..1] */
	double g; /* green [0..1] */
	double b; /* blue  [0..1] */
} RGB;

typedef struct {
	double h; /* hue        [0..360] */
	double s; /* saturation [0..1]   */
	double v; /* value      [0..1]   */
} HSV;

typedef struct {
	double h; /* hue        [0..360] */
	double s; /* saturation [0..1]   */
	double l; /* lightness  [0..1]   */
} HSL;

#define RGB(R, G, B) ((RGB){R, G, B})
#define HSV(H, S, V) ((HSV){H, S, V})
#define HSL(H, S, L) ((HSL){H, S, L})

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

double wrap(double min, double max, double value);
double clip(double min, double max, double value);

void clamp_rgb(RGB* colour);
void clamp_hsv(HSV* colour);
void clamp_hsl(HSL* colour);

RGB hsv_to_rgb(HSV colour);
RGB hsl_to_rgb(HSL colour);
HSV hsl_to_hsv(HSL colour);
HSV rgb_to_hsv(RGB colour);
HSL rgb_to_hsl(RGB colour);
HSL hsv_to_hsl(HSV colour);

#endif

#ifdef TOO_MANY_COLOURS_IMPLEMENTATION

#include <math.h>

double wrap(double min, double max, double value) {
	double delta = max - min;
	if (value < min) return value + ceil((min - value)/delta)*delta;
	if (value > max) return value - ceil((value - max)/delta)*delta;
	return value;
}

double clip(double min, double max, double value) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

void clamp_rgb(RGB* colour) {
	colour->r = clip(0.0, 1.0, colour->r);
	colour->g = clip(0.0, 1.0, colour->g);
	colour->b = clip(0.0, 1.0, colour->b);
}

void clamp_hsv(HSV* colour) {
	colour->h = wrap(0.0, 360.0, colour->h);
	colour->s = clip(0.0, 1.0, colour->s);
	colour->v = clip(0.0, 1.0, colour->v);
}

void clamp_hsl(HSL* colour) {
	colour->h = wrap(0.0, 360.0, colour->h);
	colour->s = clip(0.0, 1.0, colour->s);
	colour->l = clip(0.0, 1.0, colour->l);
}

RGB hsv_to_rgb(HSV colour) {
	clamp_hsv(&colour);

	double c = colour.v * colour.s;
	double x = c * (60.0 - fabs(wrap(0.0, 120.0, colour.h) - 60.0)) / 60.0;

	double r, g, b;
	if (colour.h < 60.0)       { r = c; g = x; b = 0; }
	else if (colour.h < 120.0) { r = x; g = c; b = 0; }
	else if (colour.h < 180.0) { r = 0; g = c; b = x; }
	else if (colour.h < 240.0) { r = 0; g = x; b = c; }
	else if (colour.h < 300.0) { r = x; g = 0; b = c; }
	else                       { r = c; g = 0; b = x; }

	double m = colour.v - c;
	RGB result = RGB(r+m, g+m, b+m);
	clamp_rgb(&result);
	return result;
}

RGB hsl_to_rgb(HSL colour) {
	clamp_hsl(&colour);

	double c = (1.0 - fabs(colour.l*2.0 - 1.0)) * colour.s;
	double x = c * (60.0 - fabs(wrap(0.0, 120.0, colour.h) - 60.0)) / 60.0;

	double r, g, b;
	if (colour.h < 60.0)       { r = c; g = x; b = 0; }
	else if (colour.h < 120.0) { r = x; g = c; b = 0; }
	else if (colour.h < 180.0) { r = 0; g = c; b = x; }
	else if (colour.h < 240.0) { r = 0; g = x; b = c; }
	else if (colour.h < 300.0) { r = x; g = 0; b = c; }
	else                       { r = c; g = 0; b = x; }

	double m = colour.l - c/2.0;
	RGB result = RGB(r+m, g+m, b+m);
	clamp_rgb(&result);
	return result;
}

HSV hsl_to_hsv(HSL colour) {
	HSV result;
	result.h = colour.h;
	result.v = colour.l + colour.s * MIN(colour.l, 1.0 - colour.l);
	if (result.v == 0.0) result.s = 0.0;
	else result.s = 2.0 * (1.0 - colour.l / result.v);

	clamp_hsv(&result);
	return result;
}

HSV rgb_to_hsv(RGB colour) {
	clamp_rgb(&colour);

	double max = MAX(MAX(colour.r, colour.g), colour.b);
	double min = MIN(MIN(colour.r, colour.g), colour.b);

	HSV result;
	result.v = max;
	double c = max - min;

	if (c == 0.0) result.h = 0;
	else if (max == colour.r) result.h = 60 * wrap(0.0, 6.0, (colour.g - colour.b)/c + 0.0);
	else if (max == colour.g) result.h = 60 * wrap(0.0, 6.0, (colour.b - colour.r)/c + 2.0);
	else if (max == colour.b) result.h = 60 * wrap(0.0, 6.0, (colour.r - colour.g)/c + 4.0);

	result.s = 0.0;
	if (result.v != 0.0) result.s = c / result.v;

	clamp_hsv(&result);
	return result;
}

HSL rgb_to_hsl(RGB colour) {
	clamp_rgb(&colour);

	double max = MAX(MAX(colour.r, colour.g), colour.b);
	double min = MIN(MIN(colour.r, colour.g), colour.b);

	HSL result;
	result.l = (max + min) / 2.0;
	double c = max - min;

	if (c == 0.0) result.h = 0;
	else if (max == colour.r) result.h = 60 * wrap(0.0, 6.0, (colour.g - colour.b)/c + 0.0);
	else if (max == colour.g) result.h = 60 * wrap(0.0, 6.0, (colour.b - colour.r)/c + 2.0);
	else if (max == colour.b) result.h = 60 * wrap(0.0, 6.0, (colour.r - colour.g)/c + 4.0);

	if (result.l == 0.0 || result.l == 1.0) result.s = 0.0;
	else result.s = (max - result.l) / MIN(result.l, 1.0 - result.l);

	clamp_hsl(&result);
	return result;
}

HSL hsv_to_hsl(HSV colour) {
	HSL result;
	result.h = colour.h;
	result.l = colour.v * (1 - colour.s / 2.0);
	if (result.l == 0.0 || result.l == 1.0) result.s = 0.0;
	else result.s = (colour.v - result.l) / MIN(result.l, 1.0 - result.l);

	clamp_hsl(&result);
	return result;
}

#endif
