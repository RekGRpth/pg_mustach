-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mustach" to load this file. \quit

CREATE OR REPLACE FUNCTION mustach(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_cjson(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_cjson' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_cjson(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_cjson' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_jansson(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_jansson' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_jansson(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_jansson' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_json_c(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_json_c(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';
