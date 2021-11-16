\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
COPY (
SELECT 1, 'Falsey', mustach('{"boolean": false}', E'\"{{^boolean}}This should be rendered.{{/boolean}}\"')
UNION
SELECT 2, 'Truthy', mustach('{"boolean": true}', E'\"{{^boolean}}This should not be rendered.{{/boolean}}\"')
UNION
SELECT 3, 'Null is falsey', mustach('{"null": null}', E'\"{{^null}}This should be rendered.{{/null}}\"')
UNION
SELECT 4, 'Context', mustach('{"context": {"name": "Joe"}}', E'\"{{^context}}Hi {{name}}.{{/context}}\"')
UNION
SELECT 5, 'List', mustach('{"list": [{"n": 1}, {"n": 2}, {"n": 3}]}', E'\"{{^list}}{{n}}{{/list}}\"')
UNION
SELECT 6, 'Empty List', mustach('{"list": []}', E'\"{{^list}}Yay lists!{{/list}}\"')
UNION
SELECT 7, 'Doubled', mustach('{"bool": false, "two": "second"}', E'{{^bool}}\n* first\n{{/bool}}\n* {{two}}\n{{^bool}}\n* third\n{{/bool}}\n')
UNION
SELECT 8, 'Nested (Falsey)', mustach('{"bool": false}', E'| A {{^bool}}B {{^bool}}C{{/bool}} D{{/bool}} E |')
UNION
SELECT 9, 'Nested (Truthy)', mustach('{"bool": true}', E'| A {{^bool}}B {{^bool}}C{{/bool}} D{{/bool}} E |')
UNION
SELECT 10, 'Context Misses', mustach('{}', E'[{{^missing}}Cannot find key ''missing''!{{/missing}}]')
UNION
SELECT 11, 'Dotted Names - Truthy', mustach('{"a": {"b": {"c": true}}}', E'\"{{^a.b.c}}Not Here{{/a.b.c}}\" == \"\"')
UNION
SELECT 12, 'Dotted Names - Falsey', mustach('{"a": {"b": {"c": false}}}', E'\"{{^a.b.c}}Not Here{{/a.b.c}}\" == \"Not Here\"')
UNION
SELECT 13, 'Dotted Names - Broken Chains', mustach('{"a": {}}', E'\"{{^a.b.c}}Not Here{{/a.b.c}}\" == \"Not Here\"')
UNION
SELECT 14, 'Surrounding Whitespace', mustach('{"boolean": false}', E' | {{^boolean}}\t|\t{{/boolean}} | \n')
UNION
SELECT 15, 'Internal Whitespace', mustach('{"boolean": false}', E' | {{^boolean}} {{! Important Whitespace }}\n {{/boolean}} | \n')
UNION
SELECT 16, 'Indented Inline Sections', mustach('{"boolean": false}', E' {{^boolean}}NO{{/boolean}}\n {{^boolean}}WAY{{/boolean}}\n')
UNION
SELECT 17, 'Standalone Lines', mustach('{"boolean": false}', E'| This Is\n{{^boolean}}\n|\n{{/boolean}}\n| A Line\n')
UNION
SELECT 18, 'Standalone Indented Lines', mustach('{"boolean": false}', E'| This Is\n  {{^boolean}}\n|\n  {{/boolean}}\n| A Line\n')
UNION
SELECT 19, 'Standalone Line Endings', mustach('{"boolean": false}', E'|\r\n{{^boolean}}\r\n{{/boolean}}\r\n|')
UNION
SELECT 20, 'Standalone Without Previous Line', mustach('{"boolean": false}', E'  {{^boolean}}\n^{{/boolean}}\n/')
UNION
SELECT 21, 'Standalone Without Newline', mustach('{"boolean": false}', E'^{{^boolean}}\n/\n  {{/boolean}}')
UNION
SELECT 22, 'Padding', mustach('{"boolean": false}', E'|{{^ boolean }}={{/ boolean }}|')
ORDER BY 1
) TO stdout;
ROLLBACK;
