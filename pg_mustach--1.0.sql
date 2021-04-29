-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mustach" to load this file. \quit

CREATE OR REPLACE FUNCTION mustach(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_cjson(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_cjson' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_cjson(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_cjson' LANGUAGE 'c';
