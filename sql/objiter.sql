\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'basic key/value iteration', mustach('{"s":{"a":1,"b":true}}', '{{#s.*}} {{*}}:{{.}}{{/s.*}}');
SELECT 2, 'empty object iterates to nothing', mustach('{"s":{}}', '[{{#s.*}}{{*}}{{/s.*}}]');
SELECT 3, 'scalar is not iterable as object', mustach('{"s":5}', '[{{#s.*}}{{*}}{{/s.*}}]');
SELECT 4, 'array is not iterable as object', mustach('{"s":[1,2]}', '[{{#s.*}}{{*}}{{/s.*}}]');
SELECT 5, 'inverted section on object iteration key', mustach('{"s":{}}', '[{{^s.*}}empty{{/s.*}}]');
SELECT 6, 'nested section reaches outer objiter key', mustach('{"s":{"a":{"x":1},"b":{"x":2}}}', '{{#s.*}}{{#.}}{{*}}={{x}} {{/.}}{{/s.*}}');
SELECT 7, 'object iteration nested inside array iteration', mustach('{"list":[{"s":{"a":1,"b":2}}]}', '{{#list}}{{#s.*}}{{*}}:{{.}};{{/s.*}}{{/list}}');
ROLLBACK;
