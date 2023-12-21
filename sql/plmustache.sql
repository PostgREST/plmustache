CREATE FUNCTION plmustache_handler()
RETURNS language_handler
LANGUAGE C AS 'plmustache';

CREATE FUNCTION plmustache_inline_handler(internal)
RETURNS void
LANGUAGE C AS 'plmustache';

CREATE FUNCTION plmustache_validator(oid)
RETURNS void
LANGUAGE C AS 'plmustache';

CREATE TRUSTED LANGUAGE plmustache
HANDLER plmustache_handler
INLINE plmustache_inline_handler
VALIDATOR plmustache_validator;
