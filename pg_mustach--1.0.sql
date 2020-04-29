-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mustach" to load this file. \quit

CREATE OR REPLACE FUNCTION json2mustach(json JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'json2mustach' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION json2mustach(json JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'json2mustach' LANGUAGE 'c';
