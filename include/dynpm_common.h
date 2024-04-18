/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#ifndef _DYNPM_COMMON_H
#define _DYNPM_COMMON_H

#include <stdio.h>
#include <stdarg.h>

// currently this value matches libevent
#define DYNPM_MAX_MESSAGE_SIZE   4096

// max. connections from a client to servers
#define MAX_SERVER_CONNECTIONS   64

// malleability modes
#define DYNPM_ACTIVE_FLAG        0b00001
#define DYNPM_PASSIVE_FLAG       0b00010
#define DYNPM_OFFERLISTENER_FLAG 0b00100
#define DYNPM_USERMODEL_FLAG     0b01000
#define DYNPM_SLURMFORMAT_FLAG   0b10000

// types of allocation operations for malleable applications
typedef enum  {
	COMPLETE=0,
	EXPAND,
	SHRINK,
	MIXED,
	CANCEL,
	FAULT_SHRINK,
	FAULT_REPLACE
} dynpm_allocation_operation_types;

// types of process status during malleable operations
typedef enum  {
	NEW=0,
	STAYING,
	JOINING,
	LEAVING
} dynpm_process_status;

// type of participating process
typedef enum  {
	INVALID_SOURCE=0,
	SCHEDULER,
	NODE_MANAGER,
	PROCESS_MANAGER,
	LAUNCHER,
} dynpm_source_type;

// logging API
void dynpm_init_logger(char *info, char *error);
void info_logger(const char *format, ...);
void error_logger(const char *format, ...);

#endif // common header

