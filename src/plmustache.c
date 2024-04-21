#include <postgres.h>

#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/varlena.h>

#include <utils/syscache.h>
#include <utils/lsyscache.h>
#include <funcapi.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <access/htup_details.h>

#include <mustach/mustach.h>

#include "observation.h"

PG_MODULE_MAGIC;

const char IMPLICIT_ITERATOR = '.';

typedef struct {
  char *prm_name;
  char *prm_value;
  bool enters_section;
} plmustache_param;

typedef struct {
  size_t num_params;
  plmustache_param *params;
} plmustache_ctx;

typedef struct {
  HeapTuple proc_tuple;
  Datum prosrc;
  int numargs;
  Oid* argtypes;
  char** argnames;
} plmustache_call_info;

static int
plmustache_enter_section(void *userdata, const char *name){
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;

  for(size_t i = 0; i < ctx->num_params; i++){
    plmustache_param* prm = &ctx->params[i];

    if(strcmp(prm->prm_name, name) == 0){
      return prm->enters_section;
    }
  }

  return 0;
}

// TODO these are used to handle sections, they're mandatory otherwise mustach segfaults
static int plmustache_next(void *closure){
  return MUSTACH_OK;
}

static int
plmustache_leave(void *closure){
  return MUSTACH_OK;
}

static int
plmustache_get_variable(void *userdata, const char *name, struct mustach_sbuf *sbuf){
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;

  for(size_t i = 0; i < ctx->num_params; i++){
    plmustache_param* prm = &ctx->params[i];

    if(strcmp(prm->prm_name, name) == 0){
      sbuf->value = prm->prm_value;
    }
  }

  return MUSTACH_OK;
}

static char *
datum_to_cstring(Datum datum, Oid typeoid, plmustache_obs_handler observer)
{
  HeapTuple typetuple = SearchSysCache(TYPEOID, ObjectIdGetDatum(typeoid), 0, 0, 0);
  if (!HeapTupleIsValid(typetuple))
    observer((plmustache_observation){ERROR_NO_TYPE_OID, .error_type_oid = typeoid});

  Form_pg_type pg_type_entry = (Form_pg_type) GETSTRUCT(typetuple);

  Datum ret = OidFunctionCall1(pg_type_entry->typoutput, datum);

  ReleaseSysCache(typetuple);

  return DatumGetCString(ret);
}

static plmustache_call_info
build_call_info(Oid function_oid, FunctionCallInfo fcinfo, plmustache_obs_handler observer){
  HeapTuple proc_tuple = SearchSysCache(PROCOID, ObjectIdGetDatum(function_oid), 0, 0, 0);
  if (!HeapTupleIsValid(proc_tuple))
    observer((plmustache_observation){ERROR_NO_OID, .error_function_oid = function_oid});

  bool is_null;
  Oid prorettype = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prorettype, &is_null);
  if(is_null)
    observer((plmustache_observation){ERROR_NO_RETTYPE});

  Datum prosrc = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prosrc, &is_null);
  if(is_null)
    observer((plmustache_observation){ERROR_NO_SRC});

  Oid* argtypes; char** argnames; char* argmodes;
  int numargs = get_func_arg_info(proc_tuple, &argtypes, &argnames, &argmodes);

  if(argmodes) // argmodes is non-NULL when any of the parameters are OUT or INOUT
    observer((plmustache_observation){ERROR_NOT_ONLY_IN_PARAMS});

  // This already prevents functions from being used as triggers.
  // So it's not necessary to use CALLED_AS_TRIGGER and CALLED_AS_EVENT_TRIGGER
  if(getBaseType(prorettype) != TEXTOID)
    observer((plmustache_observation){ERROR_NO_TEXT_RET});

  // when having a single unnamed parameter, the argnames are NULL
  if (!argnames && numargs == 1)
    observer((plmustache_observation){ERROR_UNNAMED_PARAMS});
  for(size_t i = 0; i < numargs; i++){
    if (strlen(argnames[i]) == 0)
      observer((plmustache_observation){ERROR_UNNAMED_PARAMS});
    if (argnames[i][0] == IMPLICIT_ITERATOR)
      observer((plmustache_observation){ERROR_PARAM_IMPLICIT_ITERATOR, .error_implicit_iterator = IMPLICIT_ITERATOR});
  }

  return (plmustache_call_info)
  { proc_tuple
  , prosrc
  , numargs
  , argtypes
  , argnames
  };
}

PG_FUNCTION_INFO_V1(plmustache_handler);
Datum plmustache_handler(PG_FUNCTION_ARGS)
{
  Oid function_oid = fcinfo->flinfo->fn_oid;

  char *mustache_result;
  int mustach_code;
  size_t mustache_result_size;
  struct mustach_itf itf = {
    .enter  = plmustache_enter_section,
    .next   = plmustache_next,
    .leave  = plmustache_leave,
    .get    = plmustache_get_variable,
  };

  plmustache_call_info call_info = build_call_info(function_oid, fcinfo, ereporter);

  char *template = TextDatumGetCString(DirectFunctionCall2(btrim, call_info.prosrc, CStringGetTextDatum("\n")));

  if(call_info.numargs > 0){
    plmustache_param *params = palloc0(sizeof(plmustache_param) * call_info.numargs);
    plmustache_ctx ctx = { call_info.numargs, params };

    for(size_t i = 0; i < call_info.numargs; i++){
      params[i].prm_name = call_info.argnames[i];
      NullableDatum arg = fcinfo->args[i];
      if(arg.isnull){
        params[i].prm_value = NULL;
        params[i].enters_section = false;
      }else{
        params[i].prm_value = datum_to_cstring(arg.value, call_info.argtypes[i], ereporter);
        if(call_info.argtypes[i] == BOOLOID)
          params[i].enters_section = DatumGetBool(arg.value);
        else
          params[i].enters_section = true;
      }
    }

    mustach_code = mustach_mem(template, 0, &itf, &ctx, 0, &mustache_result, &mustache_result_size);
  } else {
    mustach_code = mustach_mem(template, 0, &itf, NULL, 0, &mustache_result, &mustache_result_size);
  }

  if(mustach_code < 0){
    ereporter((plmustache_observation){ERROR_MUSTACH, .error_mustach_code = mustach_code});
  }

  ReleaseSysCache(call_info.proc_tuple);

  PG_RETURN_TEXT_P(cstring_to_text(mustache_result));
}

PG_FUNCTION_INFO_V1(plmustache_inline_handler);
Datum plmustache_inline_handler(PG_FUNCTION_ARGS)
{
  ereporter((plmustache_observation){ERROR_NO_DO_BLOCKS});
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(plmustache_validator);
Datum plmustache_validator(PG_FUNCTION_ARGS)
{
  Oid function_oid = PG_GETARG_OID(0);

  if (!CheckFunctionValidatorAccess(fcinfo->flinfo->fn_oid, function_oid))
      PG_RETURN_VOID();

  if (!check_function_bodies)
      PG_RETURN_VOID();

  plmustache_call_info call_info = build_call_info(function_oid, fcinfo, ereporter);

  ReleaseSysCache(call_info.proc_tuple);

  PG_RETURN_VOID();
}
