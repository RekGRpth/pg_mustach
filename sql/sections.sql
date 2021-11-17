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
SELECT 1, 'Truthy', mustach('{"boolean": true}', E'\"{{#boolean}}This should be rendered.{{/boolean}}\"')
UNION
SELECT 2, 'Falsey', mustach('{"boolean": false}', E'\"{{#boolean}}This should not be rendered.{{/boolean}}\"')
UNION
SELECT 3, 'Null is falsey', mustach('{"null": null}', E'\"{{#null}}This should not be rendered.{{/null}}\"')
UNION
SELECT 4, 'Context', mustach('{"context": {"name": "Joe"}}', E'\"{{#context}}Hi {{name}}.{{/context}}\"')
UNION
SELECT 5, 'Parent contexts', mustach('{"a": "foo", "b": "wrong", "sec": {"b": "bar"}, "c": {"d": "baz"}}', E'\"{{#sec}}{{a}}, {{b}}, {{c.d}}{{/sec}}\"')
UNION
SELECT 6, 'Variable test', mustach('{"foo": "bar"}', E'\"{{#foo}}{{.}} is {{foo}}{{/foo}}\"')
UNION
SELECT 7, 'List Contexts', mustach('{"tops": [{"tname": {"upper": "A", "lower": "a"}, "middles": [{"mname": "1", "bottoms": [{"bname": "x"}, {"bname": "y"}]}]}]}', E'{{#tops}}{{#middles}}{{tname.lower}}{{mname}}.{{#bottoms}}{{tname.upper}}{{mname}}{{bname}}.{{/bottoms}}{{/middles}}{{/tops}}')
UNION
SELECT 8, 'Deeply Nested Contexts', mustach('{"a": {"one": 1}, "b": {"two": 2}, "c": {"three": 3, "d": {"four": 4, "five": 5}}}', E'{{#a}}\n{{one}}\n{{#b}}\n{{one}}{{two}}{{one}}\n{{#c}}\n{{one}}{{two}}{{three}}{{two}}{{one}}\n{{#d}}\n{{one}}{{two}}{{three}}{{four}}{{three}}{{two}}{{one}}\n{{#five}}\n{{one}}{{two}}{{three}}{{four}}{{five}}{{four}}{{three}}{{two}}{{one}}\n{{one}}{{two}}{{three}}{{four}}{{.}}6{{.}}{{four}}{{three}}{{two}}{{one}}\n{{one}}{{two}}{{three}}{{four}}{{five}}{{four}}{{three}}{{two}}{{one}}\n{{/five}}\n{{one}}{{two}}{{three}}{{four}}{{three}}{{two}}{{one}}\n{{/d}}\n{{one}}{{two}}{{three}}{{two}}{{one}}\n{{/c}}\n{{one}}{{two}}{{one}}\n{{/b}}\n{{one}}\n{{/a}}\n')
UNION
SELECT 9, 'List', mustach('{"list": [{"item": 1}, {"item": 2}, {"item": 3}]}', E'\"{{#list}}{{item}}{{/list}}\"')
UNION
SELECT 10, 'Empty List', mustach('{"list": []}', E'\"{{#list}}Yay lists!{{/list}}\"')
UNION
SELECT 11, 'Doubled', mustach('{"bool": true, "two": "second"}', E'{{#bool}}\n* first\n{{/bool}}\n* {{two}}\n{{#bool}}\n* third\n{{/bool}}\n')
UNION
SELECT 12, 'Nested (Truthy)', mustach('{"bool": true}', E'| A {{#bool}}B {{#bool}}C{{/bool}} D{{/bool}} E |')
UNION
SELECT 13, 'Nested (Falsey)', mustach('{"bool": false}', E'| A {{#bool}}B {{#bool}}C{{/bool}} D{{/bool}} E |')
UNION
SELECT 14, 'Context Misses', mustach('{}', E'[{{#missing}}Found key ''missing''!{{/missing}}]')
UNION
SELECT 15, 'Implicit Iterator - String', mustach('{"list": ["a", "b", "c", "d", "e"]}', E'\"{{#list}}({{.}}){{/list}}\"')
UNION
SELECT 16, 'Implicit Iterator - Integer', mustach('{"list": [1, 2, 3, 4, 5]}', E'\"{{#list}}({{.}}){{/list}}\"')
UNION
SELECT 17, 'Implicit Iterator - Decimal', mustach('{"list": [1.1, 2.2, 3.3, 4.4, 5.5]}', E'\"{{#list}}({{.}}){{/list}}\"')
UNION
SELECT 18, 'Implicit Iterator - Array', mustach('{"list": [[1, 2, 3], ["a", "b", "c"]]}', E'\"{{#list}}({{#.}}{{.}}{{/.}}){{/list}}\"')
UNION
SELECT 19, 'Dotted Names - Truthy', mustach('{"a": {"b": {"c": true}}}', E'\"{{#a.b.c}}Here{{/a.b.c}}\" == \"Here\"')
UNION
SELECT 20, 'Dotted Names - Falsey', mustach('{"a": {"b": {"c": false}}}', E'\"{{#a.b.c}}Here{{/a.b.c}}\" == \"\"')
UNION
SELECT 21, 'Dotted Names - Broken Chains', mustach('{"a": {}}', E'\"{{#a.b.c}}Here{{/a.b.c}}\" == \"\"')
UNION
SELECT 22, 'Surrounding Whitespace', mustach('{"boolean": true}', E' | {{#boolean}}\t|\t{{/boolean}} | \n')
UNION
SELECT 23, 'Internal Whitespace', mustach('{"boolean": true}', E' | {{#boolean}} {{! Important Whitespace }}\n {{/boolean}} | \n')
UNION
SELECT 24, 'Indented Inline Sections', mustach('{"boolean": true}', E' {{#boolean}}YES{{/boolean}}\n {{#boolean}}GOOD{{/boolean}}\n')
UNION
SELECT 25, 'Standalone Lines', mustach('{"boolean": true}', E'| This Is\n{{#boolean}}\n|\n{{/boolean}}\n| A Line\n')
UNION
SELECT 26, 'Indented Standalone Lines', mustach('{"boolean": true}', E'| This Is\n  {{#boolean}}\n|\n  {{/boolean}}\n| A Line\n')
UNION
SELECT 27, 'Standalone Line Endings', mustach('{"boolean": true}', E'|\r\n{{#boolean}}\r\n{{/boolean}}\r\n|')
UNION
SELECT 28, 'Standalone Without Previous Line', mustach('{"boolean": true}', E'  {{#boolean}}\n#{{/boolean}}\n/')
UNION
SELECT 29, 'Standalone Without Newline', mustach('{"boolean": true}', E'#{{#boolean}}\n/\n  {{/boolean}}')
UNION
SELECT 30, 'Padding', mustach('{"boolean": true}', E'|{{# boolean }}={{/ boolean }}|')
ORDER BY 1
) TO stdout;
ROLLBACK;
