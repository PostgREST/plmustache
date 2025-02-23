#include "pg_prelude.h"
#include "observation.h"

#include <mustach/mustach.h>

// clang-format off
static const char *mustach_strerror(int mustach_code){
  switch(mustach_code){

    case MUSTACH_ERROR_SYSTEM           : return "system error";
    case MUSTACH_ERROR_UNEXPECTED_END   : return "unexpected end";
    case MUSTACH_ERROR_EMPTY_TAG        : return "empty tag";
    case MUSTACH_ERROR_TAG_TOO_LONG     : return "tag is too long";
    case MUSTACH_ERROR_BAD_SEPARATORS   : return "bad separators";
    case MUSTACH_ERROR_TOO_DEEP         : return "too deep";
    case MUSTACH_ERROR_CLOSING          : return "closing";
    case MUSTACH_ERROR_BAD_UNESCAPE_TAG : return "bad unescape tag";
    case MUSTACH_ERROR_INVALID_ITF      : return "invalid itf";
    case MUSTACH_ERROR_ITEM_NOT_FOUND   : return "item not found";
    case MUSTACH_ERROR_PARTIAL_NOT_FOUND: return "partial not found";
    case MUSTACH_ERROR_UNDEFINED_TAG    : return "undefined tag";
    default                             : return "unknown";

  }
}
// clang-format on

void ereporter(plmustache_observation o) {
  switch (o.obs_type) {
    case ERROR_NO_OID:
      ereport(ERROR, errmsg("could not find function with oid %u", o.error_function_oid));
      break;
    case ERROR_NO_RETTYPE:
      ereport(ERROR, errmsg("pg_proc.prorettype is NULL"));
      break;
    case ERROR_NO_SRC:
      ereport(ERROR, errmsg("pg_proc.prosrc is NULL"));
      break;
    case ERROR_NOT_ONLY_IN_PARAMS:
      ereport(ERROR, errmsg("plmustache can only have IN parameters"));
      break;
    case ERROR_NO_TEXT_RET:
      ereport(ERROR, errmsg("plmustache functions can only return the text type or a domain based on the text type"));
      break;
    case ERROR_UNNAMED_PARAMS:
      ereport(ERROR, errmsg("plmustache can only have named parameters"));
      break;
    case ERROR_PARAM_IMPLICIT_ITERATOR:
      ereport(ERROR, errmsg("parameters cannot be named the same as the implicit iterator '%c'", o.error_implicit_iterator));
      break;
    case ERROR_NO_DO_BLOCKS:
      ereport(ERROR, errmsg("plmustache doesn't allow DO blocks"));
      break;
    case ERROR_NO_TYPE_OID:
      ereport(ERROR, errmsg("could not find type with oid %u", o.error_type_oid));
      break;
    case ERROR_MUSTACH:
      ereport(ERROR, errmsg("plmustache template processing failed: %s", mustach_strerror(o.error_mustach_code)));
      break;
    case ERROR_NO_MULTIDIM:
      ereport(ERROR, errmsg("support for multidimensional arrays is not implemented"));
      break;
  }
}
