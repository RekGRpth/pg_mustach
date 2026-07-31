-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION pg_mustach UPDATE TO '3.0'" to load this file. \quit

DROP FUNCTION mustach(JSON, TEXT);
DROP FUNCTION mustach(JSON, TEXT, TEXT);
DROP FUNCTION mustach_cjson(JSON, TEXT);
DROP FUNCTION mustach_cjson(JSON, TEXT, TEXT);
DROP FUNCTION mustach_jansson(JSON, TEXT);
DROP FUNCTION mustach_jansson(JSON, TEXT, TEXT);
DROP FUNCTION mustach_json_c(JSON, TEXT);
DROP FUNCTION mustach_json_c(JSON, TEXT, TEXT);

CREATE FUNCTION mustach("json" JSONB, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_jsonb' LANGUAGE 'c';
CREATE FUNCTION mustach("json" JSONB, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_jsonb' LANGUAGE 'c';
