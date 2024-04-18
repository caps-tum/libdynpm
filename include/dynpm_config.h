/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#ifndef _DYNPM_CONFIGURATION_H
#define _DYNPM_CONFIGURATION_H

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_MAX_STR_LEN 16

typedef struct dynpm_process_conf_entry {
	char *name;
	int port;
	int threads;
	char *host;
} dynpm_process_conf_entry_t;

typedef struct dynpm_plugin_conf_entry {
	char *plugin_type;
	char *plugin_name;
} dynpm_plugin_conf_entry_t;

typedef struct dynpm_conf {
	int processes;
	dynpm_process_conf_entry_t  *process_entries;
	int plugins;
	dynpm_plugin_conf_entry_t *plugin_entries;
} dynpm_conf_t;

// from libc
extern char *program_invocation_name;
extern char *program_invocation_short_name;

// from src/libcommon/parser/configuration.c
extern dynpm_conf_t *dynpm_configuration;

void allocate_dynpm_conf(int, int);
int  dynpm_conf_parse();
int dynpm_conf_get_plugin_config(dynpm_plugin_conf_entry_t **, const char *);
int  dynpm_conf_get_process_config(dynpm_process_conf_entry_t **, const char *);
void dynpm_conf_print(void);
void free_dynpm_conf (void);

#endif // configuration header
