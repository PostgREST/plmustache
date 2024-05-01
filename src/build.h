#ifndef BUILD_H
#define BUILD_H

#include <mustach/mustach.h>
#include <postgres.h>
#include "observation.h"

typedef struct {
  char *prm_name;
  char *prm_value;
  bool enters_section;
} plmustache_param;

typedef struct {
  size_t num_params;
  plmustache_param *params;
  char *template;
} plmustache_ctx;

typedef struct {
  HeapTuple proc_tuple;
  Datum prosrc;
  int numargs;
  Oid* argtypes;
  char** argnames;
} plmustache_call_info;

struct mustach_itf build_mustach_itf(void);

plmustache_call_info
build_call_info(Oid function_oid, FunctionCallInfo fcinfo, plmustache_obs_handler observer);

plmustache_ctx
build_mustache_ctx(plmustache_call_info call_info, NullableDatum args[]);

plmustache_ctx free_plmustache_ctx(plmustache_ctx ctx);

#endif
