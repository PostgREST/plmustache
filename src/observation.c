#include <postgres.h>
#include <mustach/mustach.h>

#include "observation.h"

typedef struct {
  char buf[20];
} mustach_error_msg;

static mustach_error_msg get_mustach_error_msg(int mustach_code){
  switch(mustach_code){
    case MUSTACH_ERROR_SYSTEM           : return (mustach_error_msg){"system error"};
    case MUSTACH_ERROR_UNEXPECTED_END   : return (mustach_error_msg){"unexpected end"};
    case MUSTACH_ERROR_EMPTY_TAG        : return (mustach_error_msg){"empty tag"};
    case MUSTACH_ERROR_TAG_TOO_LONG     : return (mustach_error_msg){"tag is too long"};
    case MUSTACH_ERROR_BAD_SEPARATORS   : return (mustach_error_msg){"bad separators"};
    case MUSTACH_ERROR_TOO_DEEP         : return (mustach_error_msg){"too deep"};
    case MUSTACH_ERROR_CLOSING          : return (mustach_error_msg){"closing"};
    case MUSTACH_ERROR_BAD_UNESCAPE_TAG : return (mustach_error_msg){"bad unescape tag"};
    case MUSTACH_ERROR_INVALID_ITF      : return (mustach_error_msg){"invalid itf"};
    case MUSTACH_ERROR_ITEM_NOT_FOUND   : return (mustach_error_msg){"item not found"};
    case MUSTACH_ERROR_PARTIAL_NOT_FOUND: return (mustach_error_msg){"partial not found"};
    case MUSTACH_ERROR_UNDEFINED_TAG    : return (mustach_error_msg){"undefined tag"};
    default                             : return (mustach_error_msg){"unknown"};
  }
}

void ereporter(plmustache_observation o){
  switch(o.obs_type){
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
      ereport(ERROR,
        errmsg("plmustache template processing failed: %s", get_mustach_error_msg(o.error_mustach_code).buf)
      );
      break;
  }
}
