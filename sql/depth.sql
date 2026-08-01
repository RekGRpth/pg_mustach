\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'section nesting one level below the runtime depth limit renders', mustach('{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":true}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}', '{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}X{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}');
\set ON_ERROR_STOP false
SELECT 2, 'section nesting at the runtime depth limit errors', mustach('{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":true}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}', '{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}X{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}');
\set ON_ERROR_STOP true
ROLLBACK;
