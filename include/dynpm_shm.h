/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#ifndef _DYNPM_SHM_H
#define _DYNPM_SHM_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include "dynpm_common.h"

// shared memory API
// manager calls
int dynpm_shm_manager_init(int job_index, int application_index,
		int manager_shm_pages, int process_shm_pages,
		int *process_pid_list, int process_pid_count);
int dynpm_shm_manager_finalize();
int dynpm_shm_manager_write(const void *payload, int payload_bytes);
int dynpm_shm_manager_get_malleability_mode(uint32_t *mode, uint32_t *status);

// process calls
int dynpm_shm_process_attach(int job_index, int application_index,
		int manager_shm_pages, int process_shm_pages);
int dynpm_shm_process_detach();
int dynpm_shm_process_read( void **read_buffer, int *read_bytes);
int dynpm_shm_process_set_malleability_mode(uint32_t mode);

#endif // shm header
