-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mustach" to load this file. \quit

CREATE OR REPLACE FUNCTION text2mustach(json JSON, data TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'text2mustach' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION text2mustach(json JSON, data TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'text2mustach' LANGUAGE 'c';
