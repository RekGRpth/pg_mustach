-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mustach" to load this file. \quit

CREATE FUNCTION mustach_with_allextensions() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_allextensions' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_colon() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_colon' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_compare() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_compare' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_emptytag() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_emptytag' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_equal() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_equal' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_errorundefined() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_errorundefined' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_escfirstcmp() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_escfirstcmp' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_incpartial() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_incpartial' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_jsonpointer() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_jsonpointer' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_noextensions() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_noextensions' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_objectiter() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_objectiter' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_partialdatafirst() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_partialdatafirst' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;
CREATE FUNCTION mustach_with_singledot() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_singledot' LANGUAGE 'c' IMMUTABLE PARALLEL SAFE;

CREATE FUNCTION mustach_set_flags(flags int, is_local bool DEFAULT false) RETURNS int AS $$SELECT set_config('pg_mustach.flags', flags::text, is_local)::int$$ LANGUAGE 'sql';

CREATE FUNCTION mustach("json" JSONB, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_jsonb' LANGUAGE 'c';
CREATE FUNCTION mustach("json" JSONB, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_jsonb' LANGUAGE 'c';
