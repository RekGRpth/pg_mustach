CREATE EXTENSION pg_mustach;
SELECT 1, 'root object serialized via single dot, HTML-escaped', mustach('{"a":1,"b":2}', '{{.}}');
SELECT 2, 'root object serialized via single dot, unescaped', mustach('{"a":1,"b":2}', '{{{.}}}');
SELECT 3, 'root array serialized via single dot', mustach('[1,2,3]', '{{{.}}}');
SELECT 4, 'array element object serialized during iteration', mustach('{"list":[{"x":1},{"y":2}]}', '{{#list}}{{{.}}} {{/list}}');
DROP EXTENSION pg_mustach;
