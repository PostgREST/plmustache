#include <postgres.h>

#include <tcop/utility.h>
#include <utils/builtins.h>
#include <utils/varlena.h>

#include <utils/syscache.h>
#include <utils/lsyscache.h>
#include <funcapi.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_type.h>

#include <mustache.h>

PG_MODULE_MAGIC;

typedef struct {
	char   *prm_name;
	char   *prm_value;
} plmustache_param;

typedef struct {
	char   *string;
	size_t offset;
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

static void*
plmustache_calloc(size_t num, size_t size)
{
	return palloc0(num*size);
}

static void *
plmustache_realloc(void *dst, size_t size)
{
	if (dst && size)
		return repalloc(dst, size);
	else if (size)
		return palloc(size);
	else
		return dst;
}

static void
plmustache_error(mustache_api_t *api, void *data,
               size_t line, char const *error)
{
	ereport(ERROR, errmsg("error in template: %zu: %s", line, error));
}

static size_t
plmustache_strread(mustache_api_t *api, void *userdata, char *buffer, size_t buffer_size){
	char           *string;
	size_t         string_len;
	plmustache_ctx *ctx = (plmustache_ctx *)userdata;

	string     = ctx->string + ctx->offset;
	string_len = strlen(string);
	string_len = (string_len < buffer_size) ? string_len : buffer_size;

	memcpy(buffer, string, string_len);

	ctx->offset += string_len;
	return string_len;
}

static size_t
plmustache_strwrite(mustache_api_t *api, void *userdata, char const *buffer, size_t buffer_size){
	plmustache_ctx *ctx = (plmustache_ctx *)userdata;

	ctx->string = plmustache_realloc(ctx->string, ctx->offset + buffer_size + 1);

	memcpy(ctx->string + ctx->offset, buffer, buffer_size);
	ctx->string[ctx->offset + buffer_size] = '\0';

	ctx->offset += buffer_size;
	return buffer_size;
}

static size_t
plmustache_varget(mustache_api_t *api, void *userdata, mustache_token_variable_t *token){
	plmustache_ctx *ctx = (plmustache_ctx *)userdata;

	for(size_t i = 0; i < ctx->num_params; i++){
		plmustache_param* prm = &ctx->params[i];

		if(strcmp(prm->prm_name, token->text) == 0){
			char *val = prm->prm_value;
			return api->write(api, userdata, val, strlen(val));
		}
	}

	return 0; // error
}

static char *
datum_to_cstring(Datum datum, Oid typeoid)
{
	HeapTuple typetuple;
	Form_pg_type pg_type_entry;
	Datum ret;

	typetuple = SearchSysCache(TYPEOID, ObjectIdGetDatum(typeoid), 0, 0, 0);
	if (!HeapTupleIsValid(typetuple))
		elog(ERROR, "could not find type with oid %u", typeoid);

	pg_type_entry = (Form_pg_type) GETSTRUCT(typetuple);

	ret = OidFunctionCall1(pg_type_entry->typoutput, datum);

	ReleaseSysCache(typetuple);

	return DatumGetCString(ret);
}

static plmustache_call_info
validate_build_call_info(Oid function_oid, FunctionCallInfo fcinfo){
	HeapTuple proc_tuple;
	Oid prorettype;
	Datum prosrc;
	bool is_null;
	int numargs;
	Oid* argtypes;
	char** argnames;
	char* argmodes;
	plmustache_call_info call_info;

	proc_tuple = SearchSysCache(PROCOID, ObjectIdGetDatum(function_oid), 0, 0, 0);
	if (!HeapTupleIsValid(proc_tuple))
		elog(ERROR, "could not find function with oid %u", function_oid);

	prorettype = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prorettype, &is_null);
	if(is_null)
			ereport(ERROR, errmsg("pg_proc.prorettype is NULL"));

	prosrc = SysCacheGetAttr(PROCOID, proc_tuple, Anum_pg_proc_prosrc, &is_null);
	if(is_null)
		ereport(ERROR, errmsg("pg_proc.prosrc is NULL"));

	numargs = get_func_arg_info(proc_tuple, &argtypes, &argnames, &argmodes);

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
	}

	call_info.proc_tuple = proc_tuple;
	call_info.prosrc = prosrc;
	call_info.numargs = numargs;
	call_info.argtypes = argtypes;
	call_info.argnames = argnames;

	return call_info;
}


PG_FUNCTION_INFO_V1(plmustache_handler);
Datum plmustache_handler(PG_FUNCTION_ARGS)
{
	Oid function_oid = fcinfo->flinfo->fn_oid;
	plmustache_call_info call_info;
	char* src;

	mustache_template_t *template = NULL;
	mustache_api_t api = {
		.read         = &plmustache_strread,
		.write        = &plmustache_strwrite,
		.varget       = &plmustache_varget,
		.error        = &plmustache_error,
	};
	plmustache_ctx  dststr = { NULL, 0, 0, NULL };

	call_info = validate_build_call_info(function_oid, fcinfo);

	mustache_memory_setup(palloc, plmustache_realloc, plmustache_calloc, pfree);

	src = TextDatumGetCString(DirectFunctionCall2(btrim, call_info.prosrc, CStringGetTextDatum("\n")));

	if(call_info.numargs > 0){
		plmustache_param *params = palloc0(sizeof(plmustache_param) * call_info.numargs);
		plmustache_ctx srcstr = { src, 0, 0, NULL };

		for(size_t i = 0; i < call_info.numargs; i++){
			params[i].prm_name = call_info.argnames[i];
			params[i].prm_value = datum_to_cstring(fcinfo->args[i].value, call_info.argtypes[i]);
		}

		dststr.num_params = call_info.numargs;
		dststr.params = params;

		template = mustache_compile(&api, &srcstr);

		mustache_render(&api, &dststr, template);
	} else {
		plmustache_ctx srcstr = { src, 0, 0, NULL };

		template = mustache_compile(&api, &srcstr);
		mustache_render(&api, &dststr, template);
	}

	mustache_free(&api, template);

	ReleaseSysCache(call_info.proc_tuple);

	PG_RETURN_TEXT_P(cstring_to_text(dststr.string));
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
	plmustache_call_info call_info;

	if (!CheckFunctionValidatorAccess(fcinfo->flinfo->fn_oid, function_oid))
			PG_RETURN_VOID();

	if (!check_function_bodies)
			PG_RETURN_VOID();

	call_info = validate_build_call_info(function_oid, fcinfo);

	ReleaseSysCache(call_info.proc_tuple);

	PG_RETURN_VOID();
}
