#ifndef BUILD_H
#define BUILD_H

#include <mustach/mustach.h>
#include <postgres.h>
#include "observation.h"

typedef struct {
  char  *prm_name;
  char  *prm_value;
  char **prm_arr;
  size_t prm_arr_length;
  bool   enters_section;
  bool   is_array;
} plmustache_param;

typedef struct {
  size_t            num_params;
  plmustache_param *params;
  char             *section_key;
  size_t            section_idx;
  size_t            section_arr_length;
  char *template;
} plmustache_ctx;

typedef struct {
  HeapTuple      proc_tuple;
  Datum          prosrc;
  size_t         numargs;
  Oid           *argtypes;
  char         **argnames;
  NullableDatum *argvalues;
} plmustache_call_info;

extern struct mustach_itf plmustache_mustach_itf;

plmustache_call_info build_call_info(Oid function_oid, FunctionCallInfo fcinfo, plmustache_obs_handler observer);

plmustache_ctx build_mustache_ctx(plmustache_call_info call_info, plmustache_obs_handler observer);

plmustache_ctx free_plmustache_ctx(plmustache_ctx ctx);

#endif
