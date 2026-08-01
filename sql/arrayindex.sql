\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'dotted numeric index selects array element', mustach('{"list":["a","b","c"]}', '{{list.1}}');
SELECT 2, 'out-of-range index misses', mustach('{"list":["a","b","c"]}', '{{list.5}}');
SELECT 3, 'non-numeric index misses', mustach('{"list":["a","b","c"]}', '{{list.foo}}');
SELECT 4, 'negative index misses', mustach('{"list":["a","b","c"]}', '{{list.-1}}');
SELECT 5, 'trailing garbage after digits misses', mustach('{"list":["a","b","c"]}', '{{list.1abc}}');
SELECT 6, 'chained dotted index into a nested array', mustach('{"m":[["x","y"],["z"]]}', '{{m.0.1}}');
SELECT 7, 'dotted index feeds section truthiness (truthy element)', mustach('{"list":[1,2,3]}', '{{#list.0}}yes{{/list.0}}');
SELECT 8, 'dotted index feeds section truthiness (falsey element)', mustach('{"list":[0,2,3]}', '{{#list.0}}yes{{/list.0}}');
ROLLBACK;
