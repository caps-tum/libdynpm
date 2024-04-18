/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_common.h"

char *info_logger_path = NULL;
char *error_logger_path = NULL;
FILE *info_file = NULL;
FILE *error_file = NULL;

void dynpm_init_logger(char *info, char *error){
	info_file = fopen(info, "a");
	error_file = fopen(error, "a");
}

void info_logger(const char *format, ...){
	va_list args;
	va_start(args, format);
	vfprintf(info_file, format, args);
	va_end(args);
	fprintf(info_file, "\n");
	fflush(info_file);
}

void error_logger(const char *format, ...){
    va_list args;
    fprintf(error_file, "error: ");
    va_start(args, format);
    vfprintf(error_file, format, args);
    va_end(args);
    fprintf(error_file, "\n");
	fflush(error_file);
}
