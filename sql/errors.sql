\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'emptytag flag value', mustach_with_emptytag();
SELECT 2, 'equal flag value', mustach_with_equal();
SELECT 3, 'errorundefined flag value', mustach_with_errorundefined();
SELECT 4, 'escfirstcmp flag value', mustach_with_escfirstcmp();
SELECT 5, 'incpartial flag value', mustach_with_incpartial();
SELECT 6, 'jsonpointer flag value', mustach_with_jsonpointer();
SELECT 7, 'objectiter flag value', mustach_with_objectiter();
SELECT 8, 'partialdatafirst flag value', mustach_with_partialdatafirst();
SELECT 9, 'singledot flag value', mustach_with_singledot();
SELECT mustach_set_flags(mustach_with_noextensions());
\set ON_ERROR_STOP false
SELECT 10, 'empty tag errors without EmptyTag extension', mustach('{}', '{{}}');
\set ON_ERROR_STOP true
SELECT mustach_set_flags(mustach_with_allextensions());
\set ON_ERROR_STOP false
SELECT 11, 'malformed delimiter directive', mustach('{}', '{{==}}');
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 12, 'unescape tag missing closing brace', mustach('{}', '{{{name}}');
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 13, 'unmatched closing tag', mustach('{}', '{{/x}}');
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 14, 'mismatched closing tag name', mustach('{"a":{},"b":{}}', '{{#a}}{{/b}}');
\set ON_ERROR_STOP true
SELECT mustach_set_flags(mustach_with_allextensions() | mustach_with_errorundefined());
\set ON_ERROR_STOP false
SELECT 15, 'undefined tag errors with ErrorUndefined extension', mustach('{}', '{{missing}}');
\set ON_ERROR_STOP true
SELECT mustach_set_flags(mustach_with_allextensions());
\set ON_ERROR_STOP false
SELECT 16, 'excessive section nesting exceeds parser stack', mustach('{}', repeat('{{#s}}', 100));
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 17, 'self-recursive partial exceeds nesting limit', mustach('{"loop": "{{>loop}}"}', '{{>loop}}');
\set ON_ERROR_STOP true
SELECT 18, 'missing partial renders empty rather than erroring', mustach('{}', '{{>doesnotexist}}');
ROLLBACK;
