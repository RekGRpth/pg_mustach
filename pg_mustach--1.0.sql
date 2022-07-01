-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_mustach" to load this file. \quit

CREATE OR REPLACE FUNCTION mustach_with_allextensions() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_allextensions' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_colon() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_colon' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_compare() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_compare' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_emptytag() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_emptytag' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_equal() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_equal' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_errorundefined() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_errorundefined' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_escfirstcmp() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_escfirstcmp' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_incpartial() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_incpartial' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_jsonpointer() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_jsonpointer' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_noextensions() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_noextensions' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_objectiter() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_objectiter' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_partialdatafirst() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_partialdatafirst' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_with_singledot() RETURNS void AS 'MODULE_PATHNAME', 'pg_mustach_with_singledot' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach("json" JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach("json" JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_cjson("json" JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_cjson' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_cjson("json" JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_cjson' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_jansson("json" JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_jansson' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_jansson("json" JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_jansson' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION mustach_json_c("json" JSON, template TEXT) RETURNS TEXT AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';
CREATE OR REPLACE FUNCTION mustach_json_c("json" JSON, template TEXT, file TEXT) RETURNS BOOL AS 'MODULE_PATHNAME', 'pg_mustach_json_c' LANGUAGE 'c';
