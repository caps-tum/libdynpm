/*
 * Copyright (c) 2022-2024 Technical University of Munich (TUM)
 *
 * Additional copyrights may follow
 *
 */

#include "dynpm_shm.h"
#include <pmix.h>
#include <pmix_server.h>

// PMIx callbacks

static pmix_status_t connected_fn(const pmix_proc_t *proc, void *server_object,
                               pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t finalized_fn(const pmix_proc_t *proc, void *server_object,
                               pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t abort_fn(const pmix_proc_t *proc, void *server_object, int status,
                              const char msg[], pmix_proc_t procs[], size_t nprocs,
                              pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t fencenb_fn(const pmix_proc_t procs[], size_t nprocs, const pmix_info_t info[],
                                size_t ninfo, char *data, size_t ndata, pmix_modex_cbfunc_t cbfunc,
                                void *cbdata);
static pmix_status_t dmodex_fn(const pmix_proc_t *proc, const pmix_info_t info[], size_t ninfo,
                               pmix_modex_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t publish_fn(const pmix_proc_t *proc, const pmix_info_t info[], size_t ninfo,
                                pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t lookup_fn(const pmix_proc_t *proc, char **keys, const pmix_info_t info[],
                               size_t ninfo, pmix_lookup_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t unpublish_fn(const pmix_proc_t *proc, char **keys, const pmix_info_t info[],
                                  size_t ninfo, pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t spawn_fn(const pmix_proc_t *proc, const pmix_info_t job_info[], size_t ninfo,
                              const pmix_app_t apps[], size_t napps, pmix_spawn_cbfunc_t cbfunc,
                              void *cbdata);
static pmix_status_t connect_fn(const pmix_proc_t procs[], size_t nprocs, const pmix_info_t info[],
                                size_t ninfo, pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t disconnect_fn(const pmix_proc_t procs[], size_t nprocs,
                                   const pmix_info_t info[], size_t ninfo, pmix_op_cbfunc_t cbfunc,
                                   void *cbdata);
static pmix_status_t register_events_fn(pmix_status_t *codes, size_t ncodes,
                                       const pmix_info_t info[], size_t ninfo,
                                       pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t deregister_events_fn(pmix_status_t *codes, size_t ncodes, pmix_op_cbfunc_t cbfunc,
                                       void *cbdata);
static pmix_status_t notify_event_fn(pmix_status_t code, const pmix_proc_t *source,
                                  pmix_data_range_t range, pmix_info_t info[], size_t ninfo,
                                  pmix_op_cbfunc_t cbfunc, void *cbdata);
static pmix_status_t query_fn(pmix_proc_t *proct, pmix_query_t *queries, size_t nqueries,
                              pmix_info_cbfunc_t cbfunc, void *cbdata);
static void tool_connect_fn(pmix_info_t *info, size_t ninfo, pmix_tool_connection_cbfunc_t cbfunc,
                            void *cbdata);
static void log_fn(const pmix_proc_t *client, const pmix_info_t data[], size_t ndata,
                   const pmix_info_t directives[], size_t ndirs, pmix_op_cbfunc_t cbfunc,
                   void *cbdata);

static pmix_server_module_t dynpm_pmix_server_callbacks = {
	.client_connected = connected_fn,
	.client_finalized = finalized_fn,
	.abort = abort_fn,
	.fence_nb = fencenb_fn,
	.direct_modex = dmodex_fn,
	.publish = publish_fn,
	.lookup = lookup_fn,
	.unpublish = unpublish_fn,
	.spawn = spawn_fn,
	.connect = connect_fn,
	.disconnect = disconnect_fn,
	.register_events = register_events_fn,
	.deregister_events = deregister_events_fn,
	.notify_event = notify_event_fn,
	.query = query_fn,
	.tool_connected = tool_connect_fn,
	.log = log_fn
};

int dynpm_pmix_server_init(){
	int error_code;
	if (PMIX_SUCCESS != (error_code = PMIx_server_init(&dynpm_pmix_server_callbacks, NULL, 0))) {
		info_logger("PMIx_server_init failed with error %d (%s)",
				error_code, PMIx_Error_string(error_code));
		return error_code;
	} else {
		info_logger("PMIx_server_init: success");
	}
	return 0;
}

static pmix_status_t connected_fn(const pmix_proc_t *proc, void *server_object,
                               pmix_op_cbfunc_t cbfunc, void *cbdata) {
	info_logger("connected_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t finalized_fn(const pmix_proc_t *proc, void *server_object,
                               pmix_op_cbfunc_t cbfunc, void *cbdata) {
	info_logger("finalized_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t abort_fn(const pmix_proc_t *proc, void *server_object, int status,
                              const char msg[], pmix_proc_t procs[], size_t nprocs,
                              pmix_op_cbfunc_t cbfunc, void *cbdata){
	info_logger("abort_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t fencenb_fn(const pmix_proc_t procs[], size_t nprocs, const pmix_info_t info[],
                                size_t ninfo, char *data, size_t ndata, pmix_modex_cbfunc_t cbfunc,
                                void *cbdata){
	info_logger("fencenb_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t dmodex_fn(const pmix_proc_t *proc, const pmix_info_t info[], size_t ninfo,
                               pmix_modex_cbfunc_t cbfunc, void *cbdata){
	info_logger("dmodex_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t publish_fn(const pmix_proc_t *proc, const pmix_info_t info[], size_t ninfo,
                                pmix_op_cbfunc_t cbfunc, void *cbdata){
	info_logger("publish_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t lookup_fn(const pmix_proc_t *proc, char **keys, const pmix_info_t info[],
                               size_t ninfo, pmix_lookup_cbfunc_t cbfunc, void *cbdata){
	info_logger("lookup_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t unpublish_fn(const pmix_proc_t *proc, char **keys, const pmix_info_t info[],
                                  size_t ninfo, pmix_op_cbfunc_t cbfunc, void *cbdata){
	info_logger("unpublish_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t spawn_fn(const pmix_proc_t *proc, const pmix_info_t job_info[], size_t ninfo,
                              const pmix_app_t apps[], size_t napps, pmix_spawn_cbfunc_t cbfunc,
                              void *cbdata){
	info_logger("spawn_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t connect_fn(const pmix_proc_t procs[], size_t nprocs, const pmix_info_t info[],
                                size_t ninfo, pmix_op_cbfunc_t cbfunc, void *cbdata){
	info_logger("connect_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t disconnect_fn(const pmix_proc_t procs[], size_t nprocs,
                                   const pmix_info_t info[], size_t ninfo, pmix_op_cbfunc_t cbfunc,
                                   void *cbdata){
	info_logger("disconnect_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t register_events_fn(pmix_status_t *codes, size_t ncodes,
                                       const pmix_info_t info[], size_t ninfo,
                                       pmix_op_cbfunc_t cbfunc, void *cbdata){
	info_logger("register_events_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t deregister_events_fn(pmix_status_t *codes, size_t ncodes, pmix_op_cbfunc_t cbfunc,
                                       void *cbdata){
	info_logger("deregister_events_fn called");
    return PMIX_SUCCESS;
}

static pmix_status_t notify_event_fn(pmix_status_t code, const pmix_proc_t *source,
                                  pmix_data_range_t range, pmix_info_t info[], size_t ninfo,
                                  pmix_op_cbfunc_t cbfunc, void *cbdata){
	info_logger("notifi_event called");
    return PMIX_SUCCESS;
}

static pmix_status_t query_fn(pmix_proc_t *proct, pmix_query_t *queries, size_t nqueries,
                              pmix_info_cbfunc_t cbfunc, void *cbdata){
	info_logger("query_fn called");
    return PMIX_SUCCESS;
}

static void tool_connect_fn(pmix_info_t *info, size_t ninfo, pmix_tool_connection_cbfunc_t cbfunc,
                            void *cbdata){
	info_logger("tool_connect_fn called");
}

static void log_fn(const pmix_proc_t *client, const pmix_info_t data[], size_t ndata,
                   const pmix_info_t directives[], size_t ndirs, pmix_op_cbfunc_t cbfunc,
                   void *cbdata){
	info_logger("log_fn called");
}

