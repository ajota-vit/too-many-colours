#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define TOO_MANY_COLOURS_IMPLEMENTATION
#include "too_many_colours.h"

typedef enum {
	LOG_ERROR,
	LOG_WARNING,
} LogPriority;

typedef enum {
	COLOUR_FORMAT_NONE = -1,
	COLOUR_FORMAT_RGB = 0,
	COLOUR_FORMAT_HSV = 1,
	COLOUR_FORMAT_HSL = 2,
} ColourFormat;

typedef enum {
	FORMAT_NONE = -1,
	FORMAT_HEX,
	FORMAT_INT,
	FORMAT_FLOAT,
} Format;

typedef struct {
	ColourFormat format;
	union {
		double c[3];
		RGB rgb;
		HSV hsv;
		HSL hsl;
	} data;
} Colour;

int log_message(LogPriority priority, const char* fmt, ...) {
	va_list list;
	int result;

	va_start(list, fmt);
	fprintf(stderr, "\033[1m");
	if (priority == LOG_ERROR) fprintf(stderr, "\033[31mERROR: ");
	if (priority == LOG_WARNING) fprintf(stderr, "\033[33mWARNING: ");
	result = vfprintf(stderr, fmt, list);
	fprintf(stderr, "\033[0m");
	va_end(list);

	return result;
}

void convert(ColourFormat out_format, Colour* in, Colour* out) {
	switch (in->format) {
		case COLOUR_FORMAT_RGB: switch (out_format) {
			case COLOUR_FORMAT_RGB: clamp_rgb(&in->data.rgb); *out = *in; break;
			case COLOUR_FORMAT_HSV: out->data.hsv = rgb_to_hsv(in->data.rgb); out->format = COLOUR_FORMAT_HSV; break;
			case COLOUR_FORMAT_HSL: out->data.hsl = rgb_to_hsl(in->data.rgb); out->format = COLOUR_FORMAT_HSL; break;
			default: break;
		} break;
		case COLOUR_FORMAT_HSV: switch (out_format) {
			case COLOUR_FORMAT_RGB: out->data.rgb = hsv_to_rgb(in->data.hsv); out->format = COLOUR_FORMAT_RGB; break;
			case COLOUR_FORMAT_HSV: clamp_hsv(&in->data.hsv); *out = *in; break;
			case COLOUR_FORMAT_HSL: out->data.hsl = hsv_to_hsl(in->data.hsv); out->format = COLOUR_FORMAT_HSL; break;
			default: break;
		} break;
		case COLOUR_FORMAT_HSL: switch (out_format) {
			case COLOUR_FORMAT_RGB: out->data.rgb = hsl_to_rgb(in->data.hsl); out->format = COLOUR_FORMAT_RGB; break;
			case COLOUR_FORMAT_HSV: out->data.hsv = hsl_to_hsv(in->data.hsl); out->format = COLOUR_FORMAT_HSV; break;
			case COLOUR_FORMAT_HSL: clamp_hsl(&in->data.hsl); *out = *in; break;
			default: break;
		} break;
		default: break;
	}
}

ColourFormat parse_colour_format(char* string) {
	while (isspace(*string)) string += 1;
	for (char* s = string; *s != '\0'; s++) *s = tolower(*s);
	if (strncmp(string, "rgb", 3) == 0) return COLOUR_FORMAT_RGB;
	if (strncmp(string, "hsv", 3) == 0) return COLOUR_FORMAT_HSV;
	if (strncmp(string, "hsl", 3) == 0) return COLOUR_FORMAT_HSL;

	log_message(LOG_ERROR, "unrecognised colour format '%s'\n", string);
	exit(EXIT_FAILURE);
}

Format parse_format(char* string) {
	while (isspace(*string)) string += 1;
	for (char* s = string; *s != '\0'; s++) *s = tolower(*s);
	if (strncmp(string, "hex", 3) == 0) return FORMAT_HEX;
	if (strncmp(string, "int", 3) == 0) return FORMAT_INT;
	if (strncmp(string, "float", 3) == 0) return FORMAT_FLOAT;

	log_message(LOG_ERROR, "unrecognised format '%s'\n", string);
	exit(EXIT_FAILURE);
}

int hex_value(char c) {
	switch (c) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'A': case 'a': return 10;
		case 'B': case 'b': return 11;
		case 'C': case 'c': return 12;
		case 'D': case 'd': return 13;
		case 'E': case 'e': return 14;
		case 'F': case 'f': return 15;
		default: return 0;
	}
}

Colour parse_hex(const char* string) {
	while (isspace(*string)) string += 1;
	if (*string != '#' || strlen(string) < 7) {
		log_message(LOG_ERROR, "expected hex format #RRGGBB\n");
		exit(EXIT_FAILURE);
	}

	Colour colour;
	colour.format = COLOUR_FORMAT_NONE;
	colour.data.c[0] = 16.0*(double)hex_value(string[1]) + (double)hex_value(string[2]);
	colour.data.c[1] = 16.0*(double)hex_value(string[3]) + (double)hex_value(string[4]);
	colour.data.c[2] = 16.0*(double)hex_value(string[5]) + (double)hex_value(string[6]);
	return colour;
}

Colour parse_int(const char* string) {
	Colour colour;
	for (int i = 0; i < 3; i++) {
		while (isspace(*string)) string += 1;

		int value = 0;
		while (isdigit(*string)) {
			value *= 10;
			value += *string - '0';
			string += 1;
		}

		colour.data.c[i] = value;
	}

	return colour;
}

Colour parse_float(const char* string) {
	Colour colour;
	for (int i = 0; i < 3; i++) {
		while (isspace(*string)) string += 1;

		double value = 0;
		double mult = 0.1;
		while (isdigit(*string)) {
			value *= 10.0;
			value += *string - '0';
			string += 1;
		}
		if (*string == '.') {
			string += 1;
			while (isdigit(*string)) {
				value += mult * (double)(*string - '0');
				mult *= 0.1;
				string += 1;
			}
		}

		colour.data.c[i] = value;
	}

	return colour;
}

void eval_mod(char* string, Colour* colour) {
	while (isspace(*string)) string += 1;
	for (char* s = string; *s != '\0'; s++) *s = tolower(*s);

	ColourFormat original_colour_format = colour->format;
	ColourFormat mod_colour_format = COLOUR_FORMAT_NONE;

	if (strncmp(string, "rgb", 3) == 0) {
		string += 3;
		mod_colour_format = COLOUR_FORMAT_RGB;
	} else if (strncmp(string, "hsv", 3) == 0) {
		string += 3;
		mod_colour_format = COLOUR_FORMAT_HSV;
	} else if (strncmp(string, "hsl", 3) == 0) {
		string += 3;
		mod_colour_format = COLOUR_FORMAT_HSL;
	} else goto error;

	while (isspace(*string)) string += 1;
	if (*string++ != ':') goto error;
	while (isspace(*string)) string += 1;

	char comp = tolower(*string++);
	if (comp == '\0') goto error;
	while (isspace(*string)) string += 1;
	char op = tolower(*string++);
	if (op == '\0') goto error;
	while (isspace(*string)) string += 1;

	double value = 0;
	double mult = 0.1;
	while (isdigit(*string)) {
		value *= 10.0;
		value += *string - '0';
		string += 1;
	}
	if (*string == '.') {
		string += 1;
		while (isdigit(*string)) {
			value += mult * (double)(*string - '0');
			mult *= 0.1;
			string += 1;
		}
	}

	while (isspace(*string)) string += 1;
	int per = *string++ == '%';

	convert(mod_colour_format, colour, colour);

	double* var = NULL;

	if (colour->format == COLOUR_FORMAT_RGB) {
		if (comp == 'r') { var = &colour->data.rgb.r; if (!per) value /= 255.0; }
		else if (comp == 'g') { var = &colour->data.rgb.g; if (!per) value /= 255.0; }
		else if (comp == 'b') { var = &colour->data.rgb.b; if (!per) value /= 255.0; }
		else goto error;
	} else if (colour->format == COLOUR_FORMAT_HSV) {
		if (comp == 'h') { var = &colour->data.hsv.h; if (!per) value /= 1.0; }
		else if (comp == 's') { var = &colour->data.hsv.s; if (!per) value /= 100.0; }
		else if (comp == 'v') { var = &colour->data.hsv.v; if (!per) value /= 100.0; }
		else goto error;
	} else if (colour->format == COLOUR_FORMAT_HSL) {
		if (comp == 'h') { var = &colour->data.hsl.h; if (!per) value /= 1.0; }
		else if (comp == 's') { var = &colour->data.hsl.s; if (!per) value /= 100.0; }
		else if (comp == 'l') { var = &colour->data.hsl.l; if (!per) value /= 100.0; }
		else goto error;
	} else goto error;

	if (per) {
		if (op == '+') *var += *var * value / 100.0;
		else if (op == '-') *var -= *var * value / 100.0;
		else if (op == '=') {
			if (comp == 'h') value *= 360.0;
			*var = value / 100.0;
		}
	} else {
		if (op == '+') *var += value;
		else if (op == '-') *var -= value;
		else if (op == '=') *var = value;
	}

	if (colour->format == COLOUR_FORMAT_RGB) clamp_rgb(&colour->data.rgb);
	else if (colour->format == COLOUR_FORMAT_HSV) clamp_hsv(&colour->data.hsv);
	else if (colour->format == COLOUR_FORMAT_HSL) clamp_hsl(&colour->data.hsl);
	else goto error;

	convert(original_colour_format, colour, colour);

	return;
	error: {
		log_message(LOG_ERROR, "expected mod format '<colour format>:<colour component>[=|+|-][num|%%]'\n");
		exit(EXIT_FAILURE);
	}
} 

void draw_block(FILE* stream, RGB left, RGB right) {
	for (int i = 0; i < 3; i++) {
		fprintf(stream, "\033[38;2;%d;%d;%dm", (int)round(255.0 * left.r), (int)round(255.0 * left.g), (int)round(255.0 * left.b));
		fprintf(stream, "\033[48;2;%d;%d;%dm", (int)round(255.0 * left.r), (int)round(255.0 * left.g), (int)round(255.0 * left.b));
		fprintf(stream, "      \033[0m");
		fprintf(stream, "\033[38;2;%d;%d;%dm", (int)round(255.0 * right.r), (int)round(255.0 * right.g), (int)round(255.0 * right.b));
		fprintf(stream, "\033[48;2;%d;%d;%dm", (int)round(255.0 * right.r), (int)round(255.0 * right.g), (int)round(255.0 * right.b));
		fprintf(stream, "      \033[0m\n");
	}
}

void usage(const char* program) {
	printf("Usage:\n");
	printf("  %s [options]\n", program);
	printf("\n");
	printf("Options:\n");
	printf("  -ic [RGB|HSV|HSL]    input colour format\n");
	printf("  -oc [RGB|HSV|HSL]    output colour format\n");
	printf("  -if [HEX|INT|FLOAT]  input format\n");
	printf("  -of [HEX|INT|FLOAT]  output format\n");
	printf("  -i <file>            input file\n");
	printf("  -o <file>            output file\n");
	printf("  -b                   draws a coloured block with ansi escape codes\n");
	printf("  -m                   '<colour format>:<colour component>[=|+|-][num|%%]' modify different aspects of a colour\n");
	printf("\n");
}

int main(int argc, char* argv[]) {
	ColourFormat input_colour_format = COLOUR_FORMAT_NONE;
	ColourFormat output_colour_format = COLOUR_FORMAT_NONE;
	Format input_format = FORMAT_NONE;
	Format output_format = FORMAT_NONE;
	const char* input_path = NULL;
	const char* output_path = NULL;
	FILE* input_file = stdin;
	FILE* output_file = stdout;
	int block = 0;

	Colour in;
	Colour out;

	char* line = NULL;
	size_t len = 0;

	for (int i = 1; i < argc; i++) {
		char* value = NULL;

		if (strncmp(argv[i], "-h", 2) == 0 || strncmp(argv[i], "--help", 6) == 0) {
			usage(argv[0]);
			return EXIT_SUCCESS;
		} else if (strncmp(argv[i], "-ic", 3) == 0) {
			if (argv[i][3] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+3;

			if (input_colour_format != COLOUR_FORMAT_NONE)
				log_message(LOG_WARNING, "colour input format will be overriden by '%s'\n", value);
			input_colour_format = parse_colour_format(value);
		} else if (strncmp(argv[i], "-oc", 3) == 0) {
			if (argv[i][3] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+3;

			if (output_colour_format != COLOUR_FORMAT_NONE)
				log_message(LOG_WARNING, "colour output format will be overriden by '%s'\n", value);
			output_colour_format = parse_colour_format(value);
		} else if (strncmp(argv[i], "-if", 3) == 0) {
			if (argv[i][3] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+3;

			if (input_format != FORMAT_NONE)
				log_message(LOG_WARNING, "input format will be overriden by '%s'\n", value);
			input_format = parse_format(value);
		} else if (strncmp(argv[i], "-of", 3) == 0) {
			if (argv[i][3] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+3;

			if (output_format != FORMAT_NONE)
				log_message(LOG_WARNING, "output format will be overriden by '%s'\n", value);
			output_format = parse_format(value);
		} else if (strncmp(argv[i], "-i", 2) == 0) {
			if (argv[i][2] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+2;

			if (input_path != NULL) {
				log_message(LOG_ERROR, "multiple input files are not supported\n");
				return EXIT_FAILURE;
			}
			input_path = value;
		} else if (strncmp(argv[i], "-o", 2) == 0) {
			if (argv[i][2] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+2;

			if (output_path != NULL) {
				log_message(LOG_ERROR, "multiple output files are not supported\n");
				return EXIT_FAILURE;
			}
			output_path = value;
		} else if (strncmp(argv[i], "-b", 2) == 0) {
			block = 1;
		} else if (strncmp(argv[i], "-m", 2) == 0) {
			if (argv[i][2] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+2;
		} else {
			log_message(LOG_ERROR, "unrecognised option '%s'\n", argv[i]);
			return EXIT_FAILURE;
		}
	}

	if (input_colour_format == COLOUR_FORMAT_NONE || input_format == FORMAT_NONE) {
		log_message(LOG_ERROR, "input colour format and input format need to be specified\n");
		return EXIT_FAILURE;
	}

	if (output_colour_format == COLOUR_FORMAT_NONE) output_colour_format = input_colour_format;
	if (output_format == FORMAT_NONE) output_format = input_format;

	if (input_format == FORMAT_HEX && input_colour_format != COLOUR_FORMAT_RGB) {
		log_message(LOG_ERROR, "hex colour format only supported for rgb\n");
		return EXIT_FAILURE;
	}

	if (output_format == FORMAT_HEX && output_colour_format != COLOUR_FORMAT_RGB) {
		log_message(LOG_ERROR, "hex colour format only supported for rgb\n");
		return EXIT_FAILURE;
	}

	if (input_path != NULL) {
		input_file = fopen(input_path, "r");
		if (input_file == NULL) {
			log_message(LOG_ERROR, "failed to open '%s'\n", input_path);
			return EXIT_FAILURE;
		}
	}

	getline(&line, &len, input_file);
	if (input_format == FORMAT_HEX) in = parse_hex(line);
	else if (input_format == FORMAT_INT) in = parse_int(line);
	else if (input_format == FORMAT_FLOAT) in = parse_float(line);

	in.format = input_colour_format;
	if (in.format == COLOUR_FORMAT_RGB) {
		in.data.c[0] /= 255.0;
		in.data.c[1] /= 255.0;
		in.data.c[2] /= 255.0;
		clamp_rgb(&in.data.rgb);
	} else if (in.format == COLOUR_FORMAT_HSV) {
		in.data.c[1] /= 100.0;
		in.data.c[2] /= 100.0;
		clamp_hsv(&in.data.hsv);
	} else if (in.format == COLOUR_FORMAT_HSL) {
		in.data.c[1] /= 100.0;
		in.data.c[2] /= 100.0;
		clamp_hsl(&in.data.hsl);
	}

	convert(output_colour_format, &in, &out);

	for (int i = 1; i < argc; i++) {
		char* value = NULL;

		if (strncmp(argv[i], "-m", 2) == 0) {
			if (argv[i][2] == '\0' && i < argc-1) value = argv[++i];
			else value = argv[i]+2;

			eval_mod(value, &out);
		}
	}

	if (output_path != NULL) {
		output_file = fopen(output_path, "r");
		if (output_file == NULL) {
			log_message(LOG_ERROR, "failed to open '%s'\n", output_path);
			return EXIT_FAILURE;
		}
	}

	if (output_format == FORMAT_HEX) {
		if (out.format == COLOUR_FORMAT_RGB)
			fprintf(output_file, "#%02X%02X%02X\n", (int)round(255.0 * out.data.rgb.r), (int)round(255.0 * out.data.rgb.g), (int)round(255.0 * out.data.rgb.b));
	} else if (output_format == FORMAT_INT) {
		if (out.format == COLOUR_FORMAT_RGB)
			fprintf(output_file, "%d %d %d\n", (int)round(255.0 * out.data.rgb.r), (int)round(255.0 * out.data.rgb.g), (int)round(255.0 * out.data.rgb.b));
		if (out.format == COLOUR_FORMAT_HSV)
			fprintf(output_file, "%d %d %d\n", (int)round(1.0 * out.data.hsv.h), (int)round(100.0 * out.data.hsv.s), (int)round(100.0 * out.data.hsv.v));
		if (out.format == COLOUR_FORMAT_HSL)
			fprintf(output_file, "%d %d %d\n", (int)round(1.0 * out.data.hsl.h), (int)round(100.0 * out.data.hsl.s), (int)round(100.0 * out.data.hsl.l));
	} else if (output_format == FORMAT_FLOAT) {
		if (out.format == COLOUR_FORMAT_RGB)
			fprintf(output_file, "%lf %lf %lf\n", (255.0 * out.data.rgb.r), (255.0 * out.data.rgb.g), (255.0 * out.data.rgb.b));
		if (out.format == COLOUR_FORMAT_HSV)
			fprintf(output_file, "%lf %lf %lf\n", (1.0 * out.data.hsv.h), (100.0 * out.data.hsv.s), (100.0 * out.data.hsv.v));
		if (out.format == COLOUR_FORMAT_HSL)
			fprintf(output_file, "%lf %lf %lf\n", (1.0 * out.data.hsl.h), (100.0 * out.data.hsl.s), (100.0 * out.data.hsl.l));
	}

	if (block) {
		Colour left;
		Colour right;
		convert(COLOUR_FORMAT_RGB, &in, &left);
		convert(COLOUR_FORMAT_RGB, &out, &right);
		draw_block(output_file, left.data.rgb, right.data.rgb);
	}

	if (input_file != stdin) fclose(input_file);
	if (output_file != stdout) fclose(output_file);

	return EXIT_SUCCESS;
}
