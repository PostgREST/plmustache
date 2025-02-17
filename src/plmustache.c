#include "pg_prelude.h"
#include "observation.h"
#include "build.h"

#include <mustach/mustach.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(plmustache_handler);
Datum plmustache_handler(PG_FUNCTION_ARGS) {
  Oid function_oid = fcinfo->flinfo->fn_oid;

  plmustache_call_info call_info = build_call_info(function_oid, fcinfo, ereporter);

  plmustache_ctx ctx = build_mustache_ctx(call_info);

  char  *mustache_result;
  size_t mustache_result_size;
  int    mustach_code = mustach_mem(ctx.template, 0, &plmustache_mustach_itf, &ctx, 0, &mustache_result, &mustache_result_size);

  if (mustach_code < 0) {
    ereporter((plmustache_observation){ERROR_MUSTACH, .error_mustach_code = mustach_code});
  }

  ctx = free_plmustache_ctx(ctx);

  ReleaseSysCache(call_info.proc_tuple);

  PG_RETURN_TEXT_P(cstring_to_text(mustache_result));
}

PG_FUNCTION_INFO_V1(plmustache_inline_handler);
Datum plmustache_inline_handler(__attribute__((unused)) PG_FUNCTION_ARGS) {
  ereporter((plmustache_observation){ERROR_NO_DO_BLOCKS});
  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(plmustache_validator);
Datum plmustache_validator(PG_FUNCTION_ARGS) {
  Oid function_oid = PG_GETARG_OID(0);

  if (!CheckFunctionValidatorAccess(fcinfo->flinfo->fn_oid, function_oid)) PG_RETURN_VOID();

  if (!check_function_bodies) PG_RETURN_VOID();

  plmustache_call_info call_info = build_call_info(function_oid, fcinfo, ereporter);

  ReleaseSysCache(call_info.proc_tuple);

  PG_RETURN_VOID();
}
