#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#define TOO_MANY_COLOURS_IMPLEMENTATION
#include "too_many_colours.h"

typedef enum {
	COLOUR_FORMAT_NONE = 0,
	COLOUR_FORMAT_RGB,
	COLOUR_FORMAT_HSV,
	COLOUR_FORMAT_HSL,
} ColourFormat;

typedef enum {
	REPR_NONE = 0,
	REPR_INT,
	REPR_FLOAT,
	REPR_HEX,
} Repr;

typedef struct {
	double comp[3];
	RGB rgb;
	HSV hsv;
	HSL hsl;
} Colour;

int log_warning(const char* fmt, ...) {
	va_list list;
	int result;

	va_start(list, fmt);
	fprintf(stderr, "\033[1m\033[33mWARNING: ");
	result = vfprintf(stderr, fmt, list);
	fprintf(stderr, "\033[0m");
	va_end(list);

	return result;
}

int log_error(const char* fmt, ...) {
	va_list list;
	int result;

	va_start(list, fmt);
	fprintf(stderr, "\033[1m\033[31mERROR: ");
	result = vfprintf(stderr, fmt, list);
	fprintf(stderr, "\033[0m");
	va_end(list);

	return result;
}

void draw_block(FILE* stream, RGB left, RGB right) {
	for (int i = 0; i < 3; i++) {
		fprintf(stream, "\033[38;2;%d;%d;%dm", (int)(255.0 * left.r), (int)(255.0 * left.g), (int)(255.0 * left.b));
		fprintf(stream, "\033[48;2;%d;%d;%dm", (int)(255.0 * left.r), (int)(255.0 * left.g), (int)(255.0 * left.b));
		fprintf(stream, "      \033[0m");
		fprintf(stream, "\033[38;2;%d;%d;%dm", (int)(255.0 * right.r), (int)(255.0 * right.g), (int)(255.0 * right.b));
		fprintf(stream, "\033[48;2;%d;%d;%dm", (int)(255.0 * right.r), (int)(255.0 * right.g), (int)(255.0 * right.b));
		fprintf(stream, "      \033[0m\n");
	}
}

ColourFormat parse_colour_format(const char* string) {
	while (isspace(*string)) string += 1;

	if (tolower(string[0]) == 'r' && tolower(string[1]) == 'g' && tolower(string[2]) == 'b') return COLOUR_FORMAT_RGB;
	if (tolower(string[0]) == 'h' && tolower(string[1]) == 's' && tolower(string[2]) == 'v') return COLOUR_FORMAT_HSV;
	if (tolower(string[0]) == 'h' && tolower(string[1]) == 's' && tolower(string[2]) == 'l') return COLOUR_FORMAT_HSL;

	log_error("ukown colour format '%s'\n", string);
	exit(EXIT_FAILURE);
}

Repr parse_format(const char* string) {
	while (isspace(*string)) string += 1;

	if (tolower(string[0]) == 'h' && tolower(string[1]) == 'e' && tolower(string[2]) == 'x') return REPR_HEX;
	if (tolower(string[0]) == 'i' && tolower(string[1]) == 'n' && tolower(string[2]) == 't') return REPR_INT;
	if (tolower(string[0]) == 'f' && tolower(string[1]) == 'l' && tolower(string[2]) == 'o' &&
		tolower(string[3]) == 'a' && tolower(string[4]) == 't') return REPR_FLOAT;

	log_error("ukown format '%s'\n", string);
	exit(EXIT_FAILURE);
}

int digit_value(char c) {
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
	}

	log_error("invalid digit character '%c'\n", c);
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
	}

	log_error("invalid hex character '%c'\n", c);
	exit(EXIT_FAILURE);
}

void parse_hex(const char* string, Colour* colour) {
	while (isspace(*string)) string += 1;
	if (string[0] != '#') {
		log_warning("expected format #RRGGBB");
		exit(EXIT_FAILURE);
	}
	
	colour->comp[0] = 16.0*(double)hex_value(string[1]) + (double)hex_value(string[2]);
	colour->comp[1] = 16.0*(double)hex_value(string[3]) + (double)hex_value(string[4]);
	colour->comp[2] = 16.0*(double)hex_value(string[5]) + (double)hex_value(string[6]);
}

void parse_int(const char* string, Colour* colour) {
	for (int i = 0; i < 3; i++) {
		while (isspace(*string)) string += 1;

		int value = 0;
		while (isdigit(*string)) {
			value *= 10;
			value += digit_value(*string);
			string += 1;
		}

		colour->comp[i] = (double)value;
	}
}

void parse_float(const char* string, Colour* colour) {
	while (isspace(*string)) string += 1;

	for (int i = 0; i < 3; i++) {
		while (isspace(*string)) string += 1;

		double value = 0;
		while (isdigit(*string)) {
			value *= 10;
			value += digit_value(*string);
			string += 1;
		}
		if (*string == '.') {
			string += 1;
			double mult = 0.1;
			while (isdigit(*string)) {
				mult *= 0.1;
				value += digit_value(*string) * mult;
				string += 1;
			}
		}

		colour->comp[i] = (double)value;
	}
}

void eval_mod(const char* mod, ColourFormat format, RGB* rgb, HSV* hsv, HSL* hsl) {

}

int main(int argc, char* argv[]) {
	int block = 0;
	ColourFormat input_colour_format = COLOUR_FORMAT_NONE;
	ColourFormat output_colour_format = COLOUR_FORMAT_NONE;
	Repr input_format = REPR_NONE;
	Repr output_format = REPR_NONE;
	const char* mod = NULL;
	const char* input_file_path = NULL;
	const char* output_file_path = NULL;
	FILE* input_file = NULL;
	FILE* output_file = NULL;
	char* line = NULL;
	size_t len = 0;

	Colour* colour = NULL;
	RGB left = {0};
	RGB right = {0};
	RGB rgb = {0};
	HSV hsv = {0};
	HSL hsl = {0};

	for (int i = 1; i < argc; i++) {
		const char* value;
		
		if (strncmp(argv[i], "-b", 3) == 0) {
			block = 1;
		} else if (strncmp(argv[i], "-ic", 3) == 0) {
			if (argv[i][3] == '\0') value = argv[++i];
			else value = argv[i]+3;

			if (input_colour_format != COLOUR_FORMAT_NONE)
				log_warning("input colour format will be overriden by '%s'\n", value);
			input_colour_format = parse_colour_format(value);
		} else if (strncmp(argv[i], "-oc", 3) == 0) {
			if (argv[i][3] == '\0') value = argv[++i];
			else value = argv[i]+3;

			if (output_colour_format != COLOUR_FORMAT_NONE)
				log_warning("output colour format will be overriden by '%s'\n", value);
			output_colour_format = parse_colour_format(value);
		} else if (strncmp(argv[i], "-if", 3) == 0) {
			if (argv[i][3] == '\0') value = argv[++i];
			else value = argv[i]+3;

			if (input_format != REPR_NONE)
				log_warning("input format will be overriden by '%s'\n", value);
			input_format = parse_format(value);
		} else if (strncmp(argv[i], "-of", 3) == 0) {
			if (argv[i][3] == '\0') value = argv[++i];
			else value = argv[i]+3;

			if (output_format != REPR_NONE)
				log_warning("output format will be overriden by '%s'\n", value);
			output_format = parse_format(value);
		} else if (strncmp(argv[i], "-m", 2) == 0) {
			if (argv[i][2] == '\0') value = argv[++i];
			else value = argv[i]+2;

			if (mod != NULL)
				log_warning("mod will be overriden by '%s'\n", value);
			mod = value;
		} else if (strncmp(argv[i], "-i", 2) == 0) {
			if (argv[i][2] == '\0') value = argv[++i];
			else value = argv[i]+2;

			if (input_file_path != NULL) {
				log_error("Multiple input files not supported\n");
				return EXIT_FAILURE;
			}
			input_file_path = value;
		} else if (strncmp(argv[i], "-o", 2) == 0) {
			if (argv[i][2] == '\0') value = argv[++i];
			else value = argv[i]+2;

			if (output_file_path != NULL) {
				log_error("Multiple output files not supported\n");
				return EXIT_FAILURE;
			}
			output_file_path = value;
		} else {
			log_error("uknown option '%s'\n", argv[i]);
			return EXIT_FAILURE;
		}
	}

	if (output_format == REPR_NONE && output_colour_format == COLOUR_FORMAT_NONE) {
		output_colour_format = input_colour_format;
		output_format = input_format;
	}

	if (input_format == REPR_HEX && input_colour_format != COLOUR_FORMAT_RGB) {
		log_error("Hex input is only for RGB\n");
		return EXIT_FAILURE;
	}

	if (output_format == REPR_HEX && output_colour_format != COLOUR_FORMAT_RGB) {
		log_error("Hex output is only for RGB\n");
		return EXIT_FAILURE;
	}

	if (input_file_path == NULL) {
		input_file = stdin;
	} else {
		input_file = fopen(input_file_path, "r");
		if (input_file == NULL) {
			log_error("could not open input file '%s'\n", input_file_path);
			return EXIT_FAILURE;
		}
	}

	getline(&line, &len, input_file);

	switch (input_colour_format) {
		case COLOUR_FORMAT_NONE:
			log_error("colour format must be specified\n");
			return EXIT_FAILURE;
		case COLOUR_FORMAT_RGB: colour = (Colour*)&rgb; break;
		case COLOUR_FORMAT_HSV: colour = (Colour*)&hsv; break;
		case COLOUR_FORMAT_HSL: colour = (Colour*)&hsl; break;
	}

	switch (input_format) {
		case REPR_NONE: log_error("Not implemented\n"); return EXIT_FAILURE;
		case REPR_HEX: parse_hex(line, colour); break;
		case REPR_INT: parse_int(line, colour); break;
		case REPR_FLOAT: parse_float(line, colour); break;
	}

	switch (input_colour_format) {
		case COLOUR_FORMAT_NONE: break;
		case COLOUR_FORMAT_RGB:
			colour->comp[0] /= 255.0;
			colour->comp[1] /= 255.0;
			colour->comp[2] /= 255.0;
			clamp_rgb(&rgb);
			break;
		case COLOUR_FORMAT_HSV: 
			colour->comp[1] /= 100.0;
			colour->comp[2] /= 100.0;
			clamp_hsv(&hsv);
			break;
		case COLOUR_FORMAT_HSL: 
			colour->comp[1] /= 100.0;
			colour->comp[2] /= 100.0;
			clamp_hsl(&hsl);
			break;
	}

	free(line);

	if (input_colour_format == COLOUR_FORMAT_RGB) {
		if (output_colour_format == COLOUR_FORMAT_HSV) hsv = rgb_to_hsv(rgb);
		if (output_colour_format == COLOUR_FORMAT_HSL) hsl = rgb_to_hsl(rgb);
	} else if (input_colour_format == COLOUR_FORMAT_HSV) {
		if (output_colour_format == COLOUR_FORMAT_RGB) rgb = hsv_to_rgb(hsv);
		if (output_colour_format == COLOUR_FORMAT_HSL) hsl = hsv_to_hsl(hsv);
	} else if (input_colour_format == COLOUR_FORMAT_HSL) {
		if (output_colour_format == COLOUR_FORMAT_RGB) rgb = hsl_to_rgb(hsl);
		if (output_colour_format == COLOUR_FORMAT_HSV) hsv = hsl_to_hsv(hsl);
	}

	eval_mod(mod, output_colour_format, &rgb, &hsv, &hsl);

	if (output_file_path == NULL) {
		output_file = stdout;
	} else {
		output_file = fopen(output_file_path, "r");
		if (output_file == NULL) {
			log_error("could not open output file '%s'\n", output_file_path);
			return EXIT_FAILURE;
		}
	}

	if (output_format == REPR_HEX) {
		fprintf(output_file, "#%02X%02X%02X\n", (int)(255.0 * rgb.r), (int)(255.0 * rgb.g), (int)(255.0 * rgb.b));
	} else if (output_format == REPR_INT) {
		if (output_colour_format == COLOUR_FORMAT_RGB)
			fprintf(output_file, "%d %d %d\n", (int)(255.0 * rgb.r), (int)(255.0 * rgb.g), (int)(255.0 * rgb.b));
		if (output_colour_format == COLOUR_FORMAT_HSV)
			fprintf(output_file, "%d %d %d\n", (int)(hsv.h), (int)(100.0 * hsv.s), (int)(100.0 * hsv.v));
		if (output_colour_format == COLOUR_FORMAT_HSL)
			fprintf(output_file, "%d %d %d\n", (int)(hsl.h), (int)(100.0 * hsl.s), (int)(100.0 * hsl.l));
	} else if (output_format == REPR_FLOAT) {
		if (output_colour_format == COLOUR_FORMAT_RGB)
			fprintf(output_file, "%lf %lf %lf\n", (255.0 * rgb.r), (255.0 * rgb.g), (255.0 * rgb.b));
		if (output_colour_format == COLOUR_FORMAT_HSV)
			fprintf(output_file, "%lf %lf %lf\n", (hsv.h), (100.0 * hsv.s), (100.0 * hsv.v));
		if (output_colour_format == COLOUR_FORMAT_HSL)
			fprintf(output_file, "%lf %lf %lf\n", (hsl.h), (100.0 * hsl.s), (100.0 * hsl.l));
	}

	if (block) {
		switch (input_colour_format) {
			case COLOUR_FORMAT_NONE: break;
			case COLOUR_FORMAT_RGB: left = rgb; break;
			case COLOUR_FORMAT_HSV: left = hsv_to_rgb(hsv); break;
			case COLOUR_FORMAT_HSL: left = hsl_to_rgb(hsl); break;
		}

		switch (output_colour_format) {
			case COLOUR_FORMAT_NONE: break;
			case COLOUR_FORMAT_RGB: right = rgb; break;
			case COLOUR_FORMAT_HSV: right = hsv_to_rgb(hsv); break;
			case COLOUR_FORMAT_HSL: right = hsl_to_rgb(hsl); break;
		}

		draw_block(output_file, left, right);
	}

	if (input_file != stdin) fclose(input_file);
	if (output_file != stdout) fclose(output_file);

	return EXIT_SUCCESS;
}
