/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_config.h"

#include "bison.h"

dynpm_conf_t *dynpm_configuration;

extern FILE *yyin;

void allocate_dynpm_conf(int processes, int plugins){
	int entry;

	dynpm_configuration = malloc(sizeof(dynpm_conf_t));
	dynpm_configuration->processes = processes;
	dynpm_configuration->process_entries = malloc(processes*sizeof(dynpm_process_conf_entry_t));
	for(entry = 0; entry < processes; entry++){
		dynpm_configuration->process_entries[entry].name = malloc(CONFIG_MAX_STR_LEN*sizeof(char));
		dynpm_configuration->process_entries[entry].host = malloc(CONFIG_MAX_STR_LEN*sizeof(char));
	}
	dynpm_configuration->plugins = plugins;
	dynpm_configuration->plugin_entries  = malloc(plugins*sizeof(dynpm_plugin_conf_entry_t));
	for(entry = 0; entry < plugins; entry++){
		dynpm_configuration->plugin_entries[entry].plugin_type = malloc(CONFIG_MAX_STR_LEN*sizeof(char));
		dynpm_configuration->plugin_entries[entry].plugin_name = malloc(CONFIG_MAX_STR_LEN*sizeof(char));
	}
}

int dynpm_conf_parse(){
	yyin = fopen(PREFIX "/etc/dynpm.conf", "r");
	if(yyin != NULL){
		yyparse();
		fclose(yyin);
		return 0;
	} else {
		printf("unable to open dynpm.conf");
		return -1;
	}
}


void dynpm_conf_print(void){
	int entry;
	printf("processes: %d\n", dynpm_configuration->processes );
	for(entry = 0; entry < dynpm_configuration->processes; entry++){
		printf("process: %2d: %-12s: port: %5d; threads: %2d; host: %s;\n",
		     entry,
		     dynpm_configuration->process_entries[entry].name,
		     dynpm_configuration->process_entries[entry].port,
		     dynpm_configuration->process_entries[entry].threads,
		     dynpm_configuration->process_entries[entry].host);
	}
	printf("plugins: %d\n", dynpm_configuration->plugins );
	for(entry = 0; entry < dynpm_configuration->plugins; entry++){
		printf("wrapper: %2d: %12s:%-12s\n",
		     entry,
		     dynpm_configuration->plugin_entries[entry].plugin_type,
		     dynpm_configuration->plugin_entries[entry].plugin_name);
	}
}


int dynpm_conf_get_process_config(dynpm_process_conf_entry_t **config_entry, const char *name){
	int entry;
	for(entry = 0; entry < dynpm_configuration->processes; entry++){
		if(!strcmp(dynpm_configuration->process_entries[entry].name, name)){
			*config_entry = &dynpm_configuration->process_entries[entry];
			return 0;
		}
	}
	return -1;
}

int dynpm_conf_get_plugin_config(dynpm_plugin_conf_entry_t **config_entry, const char *type){
	int entry;
	for(entry = 0; entry < dynpm_configuration->plugins; entry++){
		if(!strcmp(dynpm_configuration->plugin_entries[entry].plugin_type, type)){
			*config_entry = &dynpm_configuration->plugin_entries[entry];
			return 0;
		}
	}
	return -1;
}

void free_dynpm_conf(){
	int entry;
	for(entry = 0; entry < dynpm_configuration->processes; entry++){
		free(dynpm_configuration->process_entries[entry].name);
		free(dynpm_configuration->process_entries[entry].host);
	}
	free(dynpm_configuration->process_entries);
	for(entry = 0; entry < dynpm_configuration->plugins; entry++){
		free(dynpm_configuration->plugin_entries[entry].plugin_type);
		free(dynpm_configuration->plugin_entries[entry].plugin_name);
	}
	free(dynpm_configuration->plugin_entries);
	free(dynpm_configuration);
}
