#include "pg_prelude.h"
#include "observation.h"
#include "build.h"

const char DOT = '.';

static bool is_implicit_iterator(const char *str){
  return str[0] == DOT;
}

static int plmustache_section_enter(void *userdata, const char *name) {
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;

  for (size_t i = 0; i < ctx->num_params; i++) {
    plmustache_param *prm = &ctx->params[i];

    if (strcmp(prm->prm_name, name) == 0) {
      ctx->section_key        = prm->prm_name;
      ctx->section_arr_length = prm->is_array ? prm->prm_arr_length : 0;
      ctx->section_idx        = 0;
      return prm->enters_section;
    }
  }

  return 0;
}

static int plmustache_section_next(void *userdata) {
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;
  return ctx->section_idx < ctx->section_arr_length;
}

static int plmustache_section_leave(void *userdata) {
  plmustache_ctx *ctx     = (plmustache_ctx *)userdata;
  ctx->section_key        = NULL;
  ctx->section_idx        = 0;
  ctx->section_arr_length = 0;
  return MUSTACH_OK;
}

static int plmustache_get_variable(void *userdata, const char *name, struct mustach_sbuf *sbuf) {
  plmustache_ctx *ctx = (plmustache_ctx *)userdata;

  if (is_implicit_iterator(name)) {
    for (size_t i = 0; i < ctx->num_params; i++) {
      plmustache_param *prm = &ctx->params[i];

      if (strcmp(prm->prm_name, ctx->section_key) == 0) {
        sbuf->value = prm->prm_arr[ctx->section_idx];
      }
    }
    ctx->section_idx = ctx->section_idx + 1;
  } else
    for (size_t i = 0; i < ctx->num_params; i++) {
      plmustache_param *prm = &ctx->params[i];

      if (strcmp(name, prm->prm_name) == 0) {
        sbuf->value = prm->prm_value;
      }
    }

  return MUSTACH_OK;
}

struct mustach_itf plmustache_mustach_itf = {

  .enter = plmustache_section_enter,
  .next  = plmustache_section_next,
  .leave = plmustache_section_leave,
  .get   = plmustache_get_variable

};

static char *datum_to_cstring(Datum datum, Oid typeoid) {
  Oid  out_func;
  bool is_varlena;
  getTypeOutputInfo(typeoid, &out_func, &is_varlena);

  return OidOutputFunctionCall(out_func, datum);
}

static plmustache_param *build_params(plmustache_call_info call_info, plmustache_obs_handler observer) {
  plmustache_param *params = palloc0(sizeof(plmustache_param) * call_info.numargs);

  for (size_t i = 0; i < call_info.numargs; i++) {
    params[i].prm_name            = call_info.argnames[i];
    NullableDatum arg             = call_info.argvalues[i];
    Oid           arg_type        = call_info.argtypes[i];
    Oid           array_elem_type = get_element_type(arg_type);
    bool          arg_is_array    = array_elem_type != InvalidOid;

    if (arg.isnull) {
      params[i].prm_value      = NULL;
      params[i].enters_section = false;
      params[i].is_array       = false;
    } else {
      params[i].prm_value = datum_to_cstring(arg.value, call_info.argtypes[i]);
      if (arg_type == BOOLOID)
        params[i].enters_section = DatumGetBool(arg.value);
      else
        params[i].enters_section = true;

      if (arg_is_array) {
        params[i].is_array           = true;
        ArrayType    *array          = DatumGetArrayTypeP(arg.value);
        ArrayIterator array_iterator = array_create_iterator(array, 0, NULL);
        int           arr_ndim       = ARR_NDIM(array);
        int           arr_length     = ArrayGetNItems(arr_ndim, ARR_DIMS(array));
        if (arr_ndim > 1) observer((plmustache_observation){ERROR_NO_MULTIDIM});

        if (arr_length > 0) {
          Datum value;
          bool  isnull;
          int   j                  = 0;
          params[i].prm_arr_length = arr_length;
          params[i].prm_arr        = palloc0(sizeof(char *) * arr_length);

          while (array_iterate(array_iterator, &value, &isnull)) {
            params[i].prm_arr[j] = isnull ? NULL : datum_to_cstring(value, array_elem_type);
            j++;
          }
        } else
          params[i].enters_section = false;
      }
    }
  }

  return params;
}

plmustache_ctx build_mustache_ctx(plmustache_call_info call_info, plmustache_obs_handler observer) {
  plmustache_ctx ctx = {0};

  // remove the newlines from the start and the end of the function body
  ctx.tpl = TextDatumGetCString(DirectFunctionCall2(btrim, call_info.prosrc, CStringGetTextDatum("\n")));

  if (call_info.numargs > 0) {
    ctx.num_params = call_info.numargs;
    ctx.params     = build_params(call_info, observer);
  }

  return ctx;
}

plmustache_ctx free_plmustache_ctx(plmustache_ctx ctx) {
  if (ctx.params) pfree(ctx.params);
  if (ctx.tpl) pfree(ctx.tpl);
  ctx = (plmustache_ctx){0};
  return ctx;
}

plmustache_call_info build_call_info(Oid function_oid, __attribute__((unused)) FunctionCallInfo fcinfo, plmustache_obs_handler observer) {
  HeapTuple proc_tuple = SearchSysCache(PROCOID, ObjectIdGetDatum(function_oid), 0, 0, 0);
  if (!HeapTupleIsValid(proc_tuple)) observer((plmustache_observation){ERROR_NO_OID, .error_function_oid = function_oid});

  bool is_null;
  Oid  prorettype = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prorettype, &is_null);
  if (is_null) observer((plmustache_observation){ERROR_NO_RETTYPE});

  Datum prosrc = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prosrc, &is_null);
  if (is_null) observer((plmustache_observation){ERROR_NO_SRC});

  Oid   *argtypes;
  char **argnames;
  char  *argmodes;
  size_t numargs = get_func_arg_info(proc_tuple, &argtypes, &argnames, &argmodes);

  if (argmodes) // argmodes is non-NULL when any of the parameters are OUT or INOUT
    observer((plmustache_observation){ERROR_NOT_ONLY_IN_PARAMS});

  // This already prevents functions from being used as triggers.
  // So it's not necessary to use CALLED_AS_TRIGGER and CALLED_AS_EVENT_TRIGGER
  if (getBaseType(prorettype) != TEXTOID) observer((plmustache_observation){ERROR_NO_TEXT_RET});

  // when having a single unnamed parameter, the argnames are NULL
  if (!argnames && numargs == 1) observer((plmustache_observation){ERROR_UNNAMED_PARAMS});
  for (size_t i = 0; i < numargs; i++) {
    if (strlen(argnames[i]) == 0) observer((plmustache_observation){ERROR_UNNAMED_PARAMS});
    if (is_implicit_iterator(argnames[i])) observer((plmustache_observation){ERROR_PARAM_IMPLICIT_ITERATOR, .error_implicit_iterator = DOT});
  }

  return (plmustache_call_info){proc_tuple, prosrc, numargs, argtypes, argnames, fcinfo->args};
}
