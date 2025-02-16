#ifndef OBSERVATION_H
#define OBSERVATION_H

typedef enum {
  ERROR_NO_OID = 1,
  ERROR_NO_RETTYPE,
  ERROR_NO_SRC,
  ERROR_NOT_ONLY_IN_PARAMS,
  ERROR_NO_TEXT_RET,
  ERROR_UNNAMED_PARAMS,
  ERROR_PARAM_IMPLICIT_ITERATOR

  ,
  ERROR_NO_DO_BLOCKS,
  ERROR_NO_TYPE_OID,
  ERROR_MUSTACH
} plmustache_observation_type;

typedef struct {
  plmustache_observation_type obs_type;
  union {
    Oid  error_function_oid;
    Oid  error_type_oid;
    int  error_mustach_code;
    char error_implicit_iterator;
  };
} plmustache_observation;

typedef void (*plmustache_obs_handler)(plmustache_observation obs);

void ereporter(plmustache_observation o);

#endif
