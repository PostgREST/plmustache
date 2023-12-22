#include <postgres.h>

#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/varlena.h>

#include <utils/syscache.h>
#include <utils/lsyscache.h>
#include <utils/fmgroids.h>
#include <funcapi.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>
#include <access/htup_details.h>

#include <mustach/mustach.h>

PG_MODULE_MAGIC;

#define IMPLICIT_ITERATOR "."

typedef struct {
  char buf[20];
} mustach_error_msg;

typedef struct {
  char   *prm_name;
  char   *prm_value;
  char   **prm_arr;
  size_t prm_arr_length;
  bool   enters_section;
  bool   is_array;
} plmustache_param;

typedef struct {
  size_t num_params;
  plmustache_param *params;
  char   *section_key;
  size_t section_idx;
  size_t section_arr_length;
} plmustache_ctx;

typedef struct {
  HeapTuple proc_tuple;
  Datum prosrc;
  int numargs;
  Oid* argtypes;
  char** argnames;
} plmustache_call_info;

static int
plmustache_section_enter(void *userdata, const char *name){
  elog(DEBUG2, "plmustache_section_enter");
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;

  for(size_t i = 0; i < ctx->num_params; i++){
    plmustache_param* prm = &ctx->params[i];

    if(strcmp(prm->prm_name, name) == 0){
      ctx->section_key        = prm->prm_name;
      ctx->section_arr_length = prm->is_array? prm->prm_arr_length : 0;
      ctx->section_idx        = 0;
      return prm->enters_section;
    }
  }

  return 0;
}

static int plmustache_section_next(void *userdata){
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;
  return ctx->section_idx < ctx->section_arr_length;
}

static int
plmustache_section_leave(void *userdata){
  elog(DEBUG2, "plmustache_section_leave");
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;
  ctx->section_key = NULL;
  ctx->section_idx = 0;
  ctx->section_arr_length = 0;
  return MUSTACH_OK;
}

static int
plmustache_get_variable(void *userdata, const char *name, struct mustach_sbuf *sbuf){
  elog(DEBUG2, "plmustache_get_variable");
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;

  if (strcmp(name, IMPLICIT_ITERATOR) == 0){
    for(size_t i = 0; i < ctx->num_params; i++){
        plmustache_param* prm = &ctx->params[i];

        if(strcmp(prm->prm_name,ctx->section_key) == 0){
          sbuf->value = prm->prm_arr[ctx->section_idx];
        }
    }
    ctx->section_idx = ctx->section_idx + 1;
  }
  else for(size_t i = 0; i < ctx->num_params; i++){
    plmustache_param* prm = &ctx->params[i];

    if(strcmp(name, prm->prm_name) == 0){
      sbuf->value = prm->prm_value;
    }
  }

  return MUSTACH_OK;
}

static char *
datum_to_cstring(Datum datum, Oid typeoid)
{
  HeapTuple typetuple = SearchSysCache(TYPEOID, ObjectIdGetDatum(typeoid), 0, 0, 0);
  if (!HeapTupleIsValid(typetuple))
    elog(ERROR, "could not find type with oid %u", typeoid);

  Form_pg_type pg_type_entry = (Form_pg_type) GETSTRUCT(typetuple);

  Datum ret = OidFunctionCall1(pg_type_entry->typoutput, datum);

  ReleaseSysCache(typetuple);

  return DatumGetCString(ret);
}

static plmustache_call_info
build_call_info(Oid function_oid, FunctionCallInfo fcinfo){
  HeapTuple proc_tuple = SearchSysCache(PROCOID, ObjectIdGetDatum(function_oid), 0, 0, 0);
  if (!HeapTupleIsValid(proc_tuple))
    elog(ERROR, "could not find function with oid %u", function_oid);

  bool is_null;
  Oid prorettype = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prorettype, &is_null);
  if(is_null)
      ereport(ERROR, errmsg("pg_proc.prorettype is NULL"));

  Datum prosrc = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prosrc, &is_null);
  if(is_null)
    ereport(ERROR, errmsg("pg_proc.prosrc is NULL"));

  Oid* argtypes; char** argnames; char* argmodes;
  int numargs = get_func_arg_info(proc_tuple, &argtypes, &argnames, &argmodes);

  if(argmodes) // argmodes is non-NULL when any of the parameters are OUT or INOUT
    ereport(ERROR, errmsg("plmustache can only have IN parameters"));

  // This already prevents functions from being used as triggers.
  // So it's not necessary to use CALLED_AS_TRIGGER and CALLED_AS_EVENT_TRIGGER
  if(getBaseType(prorettype) != TEXTOID)
      ereport(ERROR, errmsg("plmustache functions can only return the text type or a domain based on the text type"));

  // when having a single unnamed parameter, the argnames are NULL
  if (!argnames && numargs == 1)
    ereport(ERROR, errmsg("plmustache can only have named parameters"));
  for(size_t i = 0; i < numargs; i++){
    if (strlen(argnames[i]) == 0)
      ereport(ERROR, errmsg("plmustache can only have named parameters"));
    if (strcmp(argnames[i], IMPLICIT_ITERATOR) == 0)
      ereport(ERROR, errmsg("parameters cannot be named the same as the implicit iterator '%s'", IMPLICIT_ITERATOR));
  }

  return (plmustache_call_info)
  { proc_tuple
  , prosrc
  , numargs
  , argtypes
  , argnames
  };
}

static mustach_error_msg get_mustach_error_msg(int mustach_code){
  switch(mustach_code){
    case MUSTACH_ERROR_SYSTEM           : return (mustach_error_msg){"system error"};
    case MUSTACH_ERROR_UNEXPECTED_END   : return (mustach_error_msg){"unexpected end"};
    case MUSTACH_ERROR_EMPTY_TAG        : return (mustach_error_msg){"empty tag"};
    case MUSTACH_ERROR_TAG_TOO_LONG     : return (mustach_error_msg){"tag is too long"};
    case MUSTACH_ERROR_BAD_SEPARATORS   : return (mustach_error_msg){"bad separators"};
    case MUSTACH_ERROR_TOO_DEEP         : return (mustach_error_msg){"bad separators"};
    case MUSTACH_ERROR_CLOSING          : return (mustach_error_msg){"closing"};
    case MUSTACH_ERROR_BAD_UNESCAPE_TAG : return (mustach_error_msg){"bad unescape tag"};
    case MUSTACH_ERROR_INVALID_ITF      : return (mustach_error_msg){"invalid itf"};
    case MUSTACH_ERROR_ITEM_NOT_FOUND   : return (mustach_error_msg){"item not found"};
    case MUSTACH_ERROR_PARTIAL_NOT_FOUND: return (mustach_error_msg){"partial not found"};
    case MUSTACH_ERROR_UNDEFINED_TAG    : return (mustach_error_msg){"undefined tag"};
    default                             : return (mustach_error_msg){"unknown"};
  }
}

PG_FUNCTION_INFO_V1(plmustache_handler);
Datum plmustache_handler(PG_FUNCTION_ARGS)
{
  Oid function_oid = fcinfo->flinfo->fn_oid;

  char *mustache_result;
  int mustach_code;
  size_t mustache_result_size;
  struct mustach_itf itf = {
    .enter  = plmustache_section_enter,
    .next   = plmustache_section_next,
    .leave  = plmustache_section_leave,
    .get    = plmustache_get_variable,
  };

  plmustache_call_info call_info = build_call_info(function_oid, fcinfo);

  char *template = TextDatumGetCString(DirectFunctionCall2(btrim, call_info.prosrc, CStringGetTextDatum("\n")));

  if(call_info.numargs > 0){
    plmustache_param *params = palloc0(sizeof(plmustache_param) * call_info.numargs);
    plmustache_ctx ctx = { call_info.numargs, params, NULL, 0 };

    for(size_t i = 0; i < call_info.numargs; i++){
      params[i].prm_name = call_info.argnames[i];
      NullableDatum arg = fcinfo->args[i];
      Oid arg_type = call_info.argtypes[i];
      Oid array_elem_type = get_element_type(arg_type);
      bool arg_is_array = array_elem_type != InvalidOid;

      if(arg.isnull){
        params[i].prm_value = NULL;
        params[i].enters_section = false;
        params[i].is_array = false;
      }else{
        params[i].prm_value = datum_to_cstring(arg.value, arg_type);

        if(arg_type == BOOLOID)
          params[i].enters_section = DatumGetBool(arg.value);
        else
          params[i].enters_section = true;

        if(arg_is_array){
          params[i].is_array = true;
          ArrayType *array = DatumGetArrayTypeP(arg.value);
          ArrayIterator array_iterator = array_create_iterator(array, 0, NULL);
          int arr_ndim = ARR_NDIM(array);
          int arr_length = ArrayGetNItems(arr_ndim, ARR_DIMS(array));
          if(arr_ndim > 1)
            ereport(ERROR, errmsg("support for multidimensional arrays is not implemented"));

          if(arr_length > 0){
            Datum value; bool isnull; int j = 0;
            params[i].prm_arr_length = arr_length;
            params[i].prm_arr = palloc0(sizeof(char*) * arr_length);

            while (array_iterate(array_iterator, &value, &isnull)) {
              params[i].prm_arr[j] = isnull? NULL : datum_to_cstring(value, array_elem_type);
              j++;
            }
          } else
            params[i].enters_section = false;
        }

      }
    }

    mustach_code = mustach_mem(template, 0, &itf, &ctx, 0, &mustache_result, &mustache_result_size);
  } else {
    mustach_code = mustach_mem(template, 0, &itf, NULL, 0, &mustache_result, &mustache_result_size);
  }

  if(mustach_code < 0){
    ereport(ERROR,
      errmsg("plmustache template processing failed: %s", get_mustach_error_msg(mustach_code).buf)
    );
  }

  ReleaseSysCache(call_info.proc_tuple);

  PG_RETURN_TEXT_P(cstring_to_text(mustache_result));
}

PG_FUNCTION_INFO_V1(plmustache_inline_handler);
Datum plmustache_inline_handler(PG_FUNCTION_ARGS)
{
  ereport(ERROR, errmsg("plmustache doesn't allow DO blocks"));
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

  plmustache_call_info call_info = build_call_info(function_oid, fcinfo);

  ReleaseSysCache(call_info.proc_tuple);

  PG_RETURN_VOID();
}
