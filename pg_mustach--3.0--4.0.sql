-- complain if script is sourced in psql, rather than via ALTER EXTENSION
\echo Use "ALTER EXTENSION pg_mustach UPDATE TO '4.0'" to load this file. \quit

CREATE FUNCTION mustach_prepare(template TEXT) RETURNS BIGINT AS 'MODULE_PATHNAME', 'pg_mustach_prepare' LANGUAGE 'c';
CREATE FUNCTION mustach_render(id BIGINT, "json" JSONB) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_render' LANGUAGE 'c';
CREATE FUNCTION mustach_render(id BIGINT, "json" JSONB, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_render' LANGUAGE 'c';
CREATE FUNCTION mustach_forget(id BIGINT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_forget' LANGUAGE 'c';
