/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_config.h"

__attribute__((constructor)) void load(void) {
	if(dynpm_conf_parse()){
		fprintf(stderr, "could not parse dynpm.conf\n");
		return;
	}
}

__attribute__((destructor)) void unload(void) {
	free_dynpm_conf();
}

