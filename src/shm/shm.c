/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_shm.h"

typedef struct dynpm_shm_control {
	uint32_t read_count;
	uint32_t write_count;
	uint32_t manager_pid;
	uint32_t job_index;
	uint32_t application_index;
	uint32_t shm_mapped_bytes;
	uint32_t process_count;
	uint32_t node_status;
	uint32_t payload_max;
	uint32_t payload_bytes;
	char padding[24]; // pad to a total of 64 bytes for atomics
	uint32_t shm_fd;
	void **shm_process_base;
	void **shm_process_payload;
	void *shm_base;
	void *shm_payload;
} dynpm_shm_manager_control_t;

typedef struct dynpm_shm_process_control {
	uint32_t read_count;
	uint32_t write_count;
	uint32_t process_pid;
	uint32_t job_index;
	uint32_t application_index;
	uint32_t shm_mapped_bytes;
	uint32_t malleability_mode;
	uint32_t process_status;
	uint32_t payload_max;
	uint32_t payload_bytes;
	char padding[24]; // pad to a total of 64 bytes for atomics
	uint32_t shm_fd;
	void *shm_base;
	void *shm_payload;
} dynpm_shm_process_control_t;

// this is an array at the manager, single entry at each process
static int is_manager = 0;
static int is_process = 0;
static dynpm_shm_manager_control_t *manager_control = NULL;
static dynpm_shm_process_control_t **process_control = NULL;
static char zero_line[64] = { 0 };

static void _print_process_control(dynpm_shm_process_control_t *process){
	info_logger("read_count: %d", process->read_count);
	info_logger("write_count: %d", process->write_count);
	info_logger("porcess_pid: %d", process->process_pid);
	info_logger("job_index: %d", process->job_index);
	info_logger("application_index: %d", process->application_index);
	info_logger("shm_mapped_bytes: %d", process->shm_mapped_bytes);
	info_logger("malleability_mode: %d", process->malleability_mode);
	info_logger("process_status: %d", process->process_status);
	info_logger("payload_max: %d", process->payload_max);
	info_logger("payload_bytes: %d", process->payload_bytes);
}

static void _print_manager_control(){
	info_logger("read_count: %d", manager_control->read_count);
	info_logger("write_count: %d", manager_control->write_count);
	info_logger("porcess_pid: %d", manager_control->manager_pid);
	info_logger("job_index: %d", manager_control->job_index);
	info_logger("application_index: %d", manager_control->application_index);
	info_logger("shm_mapped_bytes: %d", manager_control->shm_mapped_bytes);
	info_logger("process_count: %d", manager_control->process_count);
	info_logger("node_status: %d", manager_control->node_status);
	info_logger("payload_max: %d", manager_control->payload_max);
	info_logger("payload_bytes: %d", manager_control->payload_bytes);
}

static int _init_manager_shm(int job_index, int application_index, int shm_pages,
		int process_count){

	char manager_shm_path[256];

	manager_control = malloc(sizeof(dynpm_shm_manager_control_t));
	manager_control->read_count = 0;
	manager_control->write_count = 0;
	manager_control->manager_pid = getpid();
	manager_control->job_index = job_index;
	manager_control->application_index = application_index;
	manager_control->shm_mapped_bytes = shm_pages * 4096;
	manager_control->process_count = process_count;
	manager_control->node_status = 0;
	manager_control->payload_max = (shm_pages * 4096) - 64;
	manager_control->payload_bytes = 0;

	// one per process at the manager
	process_control = malloc(sizeof(dynpm_shm_process_control_t *) * process_count);
	for(int i = 0; i < process_count; i++){
		process_control[i] = malloc(sizeof(dynpm_shm_process_control_t));
	}

	// TODO for node-local malleability, we may want to change this to a list
	manager_control->shm_process_base    = malloc(sizeof(void*) * process_count);
	manager_control->shm_process_payload = malloc(sizeof(void*) * process_count);

	sprintf(manager_shm_path, "/dynpm_%d_%d", job_index, application_index);

	manager_control->shm_fd = shm_open(manager_shm_path, O_CREAT | O_RDWR, 0666);
	if(manager_control->shm_fd < 0) {
		error_logger("failed to initialize manager shm as reader");
		return -3;
	}

	if(ftruncate(manager_control->shm_fd, shm_pages * 4096)){
		error_logger("failed to truncate manager shm file");
		return -1;
	}

	manager_control->shm_base = mmap(0, shm_pages * 4096,
			PROT_READ | PROT_WRITE, MAP_SHARED, manager_control->shm_fd, 0);
	if(manager_control->shm_base == MAP_FAILED){
		error_logger("failed to mmap manager shm file: %s ", strerror(errno));
		return -2;
	}

	manager_control->shm_payload = manager_control->shm_base + 64;

	//memcpy(manager_control->shm_base, zero_line, 64);
	memcpy(manager_control->shm_base, manager_control, 64);

	return 0;
}

int _init_process_shm(int job_index, int application_index, int shm_pages,
		int process_index, int process_pid){

	char process_shm_path[256];

	process_control[process_index]->read_count = 0;
	process_control[process_index]->write_count = 0;
	process_control[process_index]->process_pid = process_pid;
	process_control[process_index]->job_index = job_index;
	process_control[process_index]->application_index = application_index;
	process_control[process_index]->shm_mapped_bytes = shm_pages * 4096;
	process_control[process_index]->malleability_mode = 0;
	process_control[process_index]->process_status = 0;
	process_control[process_index]->payload_max = (shm_pages * 4096) - 64;
	process_control[process_index]->payload_bytes = 0;

	sprintf(process_shm_path, "/dynpm_%d_%d_%d",
			job_index, application_index, process_pid);

	process_control[process_index]->shm_fd =
		shm_open(process_shm_path, O_CREAT | O_RDWR, 0666);
	if(process_control[process_index]->shm_fd < 0) {
		error_logger("failed to initialize process shm");
		return -3;
	}

	if(ftruncate(process_control[process_index]->shm_fd, shm_pages * 4096)){
		error_logger("failed to truncate process shm file");
		return -1;
	}

	process_control[process_index]->shm_base = mmap(0, shm_pages * 4096,
			PROT_READ | PROT_WRITE, MAP_SHARED, process_control[process_index]->shm_fd, 0);
	if(process_control[process_index]->shm_base == MAP_FAILED){
		error_logger("failed to mmap process shm file: %s ", strerror(errno));
		return -2;
	}

	process_control[process_index]->shm_payload = process_control[process_index]->shm_base + 64;

	manager_control->shm_process_base[process_index]    = process_control[process_index]->shm_base;
	manager_control->shm_process_payload[process_index] = process_control[process_index]->shm_payload;

	//memcpy(process_control[process_index]->shm_base, zero_line, 64);
	memcpy(process_control[process_index]->shm_base,
			process_control[process_index], 64);

	return 0;
}

int dynpm_shm_manager_init(int job_index, int application_index,
		int manager_shm_pages, int process_shm_pages,
		int *pid_list, int process_count){

	int error_code;
	assert(manager_control == NULL);

	error_code = _init_manager_shm(job_index, application_index,
			manager_shm_pages, process_count);
	info_logger(" ----- manager initialized ----- ");
	_print_manager_control();

	for(int i = 0; i < process_count; i++){
		error_code += _init_process_shm(job_index, application_index,
				process_shm_pages, i, pid_list[i]);
		info_logger(" ---- process %d initialized ---- ", i);
		_print_process_control(process_control[i]);
	}
	info_logger(" -------------------------------- ");

	asm volatile ("sfence" ::: "memory");
	return error_code;
}

int dynpm_shm_process_attach(int job_index, int application_index,
		int manager_shm_pages, int process_shm_pages){

	assert(manager_control == NULL);
	uint32_t pid = getpid();

	manager_control = malloc(sizeof(dynpm_shm_manager_control_t));

	char shm_path[256];
    sprintf(shm_path, "/dynpm_%d_%d", job_index, application_index);

	manager_control->shm_fd = shm_open(shm_path, O_RDONLY, 0666);
	if(manager_control->shm_fd < 0) {
		error_logger("failed to initialize manager shm as reader");
		return -2;
	}

	manager_control->shm_base = mmap(0, manager_shm_pages * 4096,
			PROT_READ, MAP_SHARED, manager_control->shm_fd, 0);
	if(manager_control->shm_base == MAP_FAILED){
		error_logger("failed to mmap manager shm file: %s ", strerror(errno));
		return -5;
	}

	memcpy(manager_control, manager_control->shm_base, 64);
	manager_control->shm_payload = (manager_control->shm_base + 64);

	info_logger(" --- manager at pid %d --- ", pid);
	_print_manager_control();

	info_logger("looking for shm for pid: %d (%d total)",
			pid, manager_control->process_count);
	assert(process_control == NULL);

    sprintf(shm_path, "/dynpm_%d_%d_%d", job_index, application_index, pid);

	// single entry at the process level, own shm (array only at manager)
	process_control = malloc(sizeof(dynpm_shm_process_control_t*));
	*process_control = malloc(sizeof(dynpm_shm_process_control_t));

	process_control[0]->shm_fd = shm_open(shm_path, O_RDWR, 0666);
	if(process_control[0]->shm_fd < 0) {
		error_logger("failed to initialize process shm");
		return -3;
	}

	//if(ftruncate(process_control[0]->shm_fd, process_shm_pages * 4096)){
	//	error_logger("failed to truncate process shm file");
	//	return -2;
	//}

	process_control[0]->shm_base = mmap(0, process_shm_pages * 4096,
			PROT_READ | PROT_WRITE, MAP_SHARED, process_control[0]->shm_fd, 0);
	if(process_control[0]->shm_base == MAP_FAILED){
		error_logger("failed to mmap process shm file: %s ", strerror(errno));
		return -1;
	}

	memcpy(process_control[0], process_control[0]->shm_base, 64);
	process_control[0]->shm_payload = (process_control[0]->shm_base + 64);

	info_logger(" --- process %d --- ", pid);
	_print_process_control(process_control[0]);

	//asm volatile ("sfence" ::: "memory");
	return 0;
}

int dynpm_shm_manager_finalize(){

	char manager_shm_path[256];
    sprintf(manager_shm_path, "/dynpm_%d_%d",
			manager_control->job_index, manager_control->application_index);

	munmap(manager_control->shm_base, manager_control->shm_mapped_bytes);
	close(manager_control->shm_fd);
	shm_unlink(manager_shm_path);

	free(manager_control);
	manager_control = NULL;

	return 0;
}

int dynpm_shm_process_detach(){

	char manager_shm_path[256];
	sprintf(manager_shm_path, "/dynpm_%d_%d",
			manager_control->job_index, manager_control->application_index);

	munmap(manager_control->shm_base, manager_control->shm_mapped_bytes);
	close(manager_control->shm_fd);
	shm_unlink(manager_shm_path);

	free(manager_control);
	manager_control = NULL;

	return 0;
}

static int _all_processes_read_ready(){
	int ready = 0;
	for(int i = 0; i < manager_control->process_count; i++){
		//asm volatile ("lfence" ::: "memory");
		memcpy(process_control[i], process_control[i]->shm_base, 64);

		// the process has read previous manager payload: read ready
		if(process_control[i]->read_count == manager_control->write_count){
			ready++;
		}
	}

	if(ready == manager_control->process_count) return 1;
	else return 0;
}

int dynpm_shm_manager_write(const void *payload, int payload_bytes){
	if(payload == NULL){
		error_logger("NULL payload in manager_write");
		return -1;
	}

	if(payload_bytes > 0 && payload_bytes < manager_control->payload_max){
		if(_all_processes_read_ready()){
			memcpy(manager_control->shm_payload, payload, payload_bytes);
			manager_control->payload_bytes = payload_bytes;
			manager_control->write_count++;
			memcpy(manager_control->shm_base, manager_control, 64);
			asm volatile ("sfence" ::: "memory");
		} else {
			info_logger("some process are not ready; not writing...");
			return -2;
		}
	} else {
		error_logger("writing to target shm with invalid payload bytes");
		return -3;
	}

	return 0;
}

int dynpm_shm_process_read(void **read_buffer, int *read_bytes){

	memcpy(manager_control, manager_control->shm_base, 64);

	if(process_control[0]->read_count == manager_control->write_count){
		info_logger("process and manager are in sync: %d read/write counts",
				manager_control->write_count);
		*read_bytes = 0;
	} else if(process_control[0]->read_count == (manager_control->write_count - 1)){
		info_logger("%d bytes available to read", manager_control->payload_bytes);
		memcpy((*read_buffer), (manager_control->shm_payload), manager_control->payload_bytes);
		*read_bytes = manager_control->payload_bytes;
		process_control[0]->read_count++;
		memcpy(process_control[0]->shm_base, process_control[0], 64);
		asm volatile ("sfence" ::: "memory");
	} else {
		error_logger("missed message from the manager; read: %d; write: %d",
				process_control[0]->read_count, manager_control->write_count);
		return -1;
	}

	return 0;
}

int dynpm_shm_process_set_malleability_mode(uint32_t mode){
	process_control[0]->malleability_mode = mode;
	process_control[0]->write_count++;
	process_control[0]->payload_bytes = 0; // metadata update; no payload
	memcpy(process_control[0]->shm_base, process_control[0], 64);
	asm volatile ("sfence" ::: "memory");
	info_logger("mall. mode: %d", mode);
	info_logger("process_control[0]->malleability_mode: %d",
			process_control[0]->malleability_mode);
	return 0;
}

static int _sync_processes_meta(){
	int meta_updates = 0;
	for(int i = 0; i < manager_control->process_count; i++){
		memcpy(process_control[i], process_control[i]->shm_base, 64);

		// the process has updated metadata not read by the manager
		if(process_control[i]->write_count == (manager_control->read_count - 1)
				&& process_control[i]->payload_bytes == 0){
			meta_updates++;
		}
	}

	return meta_updates;
}

int dynpm_shm_manager_get_malleability_mode(uint32_t *mode, uint32_t *status){

	int meta_updates = _sync_processes_meta();
	info_logger("meta_updates: %d", meta_updates);

	if(meta_updates == manager_control->process_count || meta_updates == 0){
		if(meta_updates > 0){
			manager_control->read_count++;
			memcpy(manager_control->shm_base, manager_control, 64);
			asm volatile ("sfence" ::: "memory");
		}

		// metadata is up to date; check on reported modes
		uint32_t first_mode = process_control[0]->malleability_mode;
		info_logger("first_mode: %d", first_mode);
		int mode_mismatch = 0;
		for(int i = 1; i < manager_control->process_count; i++){
			if(first_mode != process_control[i]->malleability_mode){
				mode_mismatch = 1;
				break;
			}
		}
		if(mode_mismatch){
			*mode = 0; // malleability disabled
			*status = -2; // sync, but no agreement on the mode
			return -1;
		} else {
			*mode = first_mode; // may be disabled, or a combination of flags
			*status = 0; // matching modes
		}
	} else {
		*mode = 0; // malleability disabled
		*status = -1; // out of sync; need to repoll or give up
		return -1;
	}

	return 0;
}

