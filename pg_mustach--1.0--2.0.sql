-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION pg_mustach UPDATE TO '2.0'" to load this file. \quit

DROP FUNCTION mustach_with_allextensions();
DROP FUNCTION mustach_with_colon();
DROP FUNCTION mustach_with_compare();
DROP FUNCTION mustach_with_emptytag();
DROP FUNCTION mustach_with_equal();
DROP FUNCTION mustach_with_errorundefined();
DROP FUNCTION mustach_with_escfirstcmp();
DROP FUNCTION mustach_with_incpartial();
DROP FUNCTION mustach_with_jsonpointer();
DROP FUNCTION mustach_with_noextensions();
DROP FUNCTION mustach_with_objectiter();
DROP FUNCTION mustach_with_partialdatafirst();
DROP FUNCTION mustach_with_singledot();

DROP FUNCTION mustach(JSON, TEXT);
DROP FUNCTION mustach(JSON, TEXT, TEXT);
DROP FUNCTION mustach_cjson(JSON, TEXT);
DROP FUNCTION mustach_cjson(JSON, TEXT, TEXT);
DROP FUNCTION mustach_jansson(JSON, TEXT);
DROP FUNCTION mustach_jansson(JSON, TEXT, TEXT);
DROP FUNCTION mustach_json_c(JSON, TEXT);
DROP FUNCTION mustach_json_c(JSON, TEXT, TEXT);

CREATE FUNCTION mustach_with_allextensions() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_allextensions' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_colon() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_colon' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_compare() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_compare' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_emptytag() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_emptytag' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_equal() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_equal' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_errorundefined() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_errorundefined' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_escfirstcmp() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_escfirstcmp' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_incpartial() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_incpartial' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_jsonpointer() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_jsonpointer' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_noextensions() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_noextensions' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_objectiter() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_objectiter' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_partialdatafirst() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_partialdatafirst' LANGUAGE 'c' IMMUTABLE;
CREATE FUNCTION mustach_with_singledot() RETURNS int AS 'MODULE_PATHNAME', 'pg_mustach_with_singledot' LANGUAGE 'c' IMMUTABLE;

CREATE FUNCTION mustach_set_flags(flags int, is_local bool DEFAULT false) RETURNS int AS $$SELECT set_config('pg_mustach.flags', flags::text, is_local)::int$$ LANGUAGE 'sql';

CREATE FUNCTION mustach("json" JSONB, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_jsonb' LANGUAGE 'c';
CREATE FUNCTION mustach("json" JSONB, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_jsonb' LANGUAGE 'c';

CREATE FUNCTION mustach_prepare(template TEXT, tplname NAME DEFAULT NULL) RETURNS VOID AS 'MODULE_PATHNAME', 'pg_mustach_prepare' LANGUAGE 'c';
CREATE FUNCTION mustach_render("json" JSONB, tplname NAME DEFAULT NULL) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_render' LANGUAGE 'c';
CREATE FUNCTION mustach_render("json" JSONB, file TEXT, tplname NAME DEFAULT NULL) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_render' LANGUAGE 'c';
CREATE FUNCTION mustach_forget(tplname NAME DEFAULT NULL) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_forget' LANGUAGE 'c';
