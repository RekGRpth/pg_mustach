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
SELECT 1, 'No Interpolation', mustach('{}', E'Hello from {Mustache}!\n')
UNION
SELECT 2, 'Basic Interpolation', mustach('{"subject": "world"}', E'Hello, {{subject}}!\n')
UNION
SELECT 3, 'HTML Escaping', mustach('{"forbidden": "& \" < >"}', E'These characters should be HTML escaped: {{forbidden}}\n')
UNION
SELECT 4, 'Triple Mustache', mustach('{"forbidden": "& \" < >"}', E'These characters should not be HTML escaped: {{{forbidden}}}\n')
UNION
SELECT 5, 'Ampersand', mustach('{"forbidden": "& \" < >"}', E'These characters should not be HTML escaped: {{&forbidden}}\n')
UNION
SELECT 6, 'Basic Integer Interpolation', mustach('{"mph": 85}', E'\"{{mph}} miles an hour!\"')
UNION
SELECT 7, 'Triple Mustache Integer Interpolation', mustach('{"mph": 85}', E'\"{{{mph}}} miles an hour!\"')
UNION
SELECT 8, 'Ampersand Integer Interpolation', mustach('{"mph": 85}', E'\"{{&mph}} miles an hour!\"')
UNION
SELECT 9, 'Basic Decimal Interpolation', mustach('{"power": 1.21}', E'\"{{power}} jiggawatts!\"')
UNION
SELECT 10, 'Triple Mustache Decimal Interpolation', mustach('{"power": 1.21}', E'\"{{{power}}} jiggawatts!\"')
UNION
SELECT 11, 'Ampersand Decimal Interpolation', mustach('{"power": 1.21}', E'\"{{&power}} jiggawatts!\"')
UNION
SELECT 12, 'Basic Null Interpolation', mustach('{"cannot": null}', E'I ({{cannot}}) be seen!')
UNION
SELECT 13, 'Triple Mustache Null Interpolation', mustach('{"cannot": null}', E'I ({{{cannot}}}) be seen!')
UNION
SELECT 14, 'Ampersand Null Interpolation', mustach('{"cannot": null}', E'I ({{&cannot}}) be seen!')
UNION
SELECT 15, 'Basic Context Miss Interpolation', mustach('{}', E'I ({{cannot}}) be seen!')
UNION
SELECT 16, 'Triple Mustache Context Miss Interpolation', mustach('{}', E'I ({{{cannot}}}) be seen!')
UNION
SELECT 17, 'Ampersand Context Miss Interpolation', mustach('{}', E'I ({{&cannot}}) be seen!')
UNION
SELECT 18, 'Dotted Names - Basic Interpolation', mustach('{"person": {"name": "Joe"}}', E'\"{{person.name}}\" == \"{{#person}}{{name}}{{/person}}\"')
UNION
SELECT 19, 'Dotted Names - Triple Mustache Interpolation', mustach('{"person": {"name": "Joe"}}', E'\"{{{person.name}}}\" == \"{{#person}}{{{name}}}{{/person}}\"')
UNION
SELECT 20, 'Dotted Names - Ampersand Interpolation', mustach('{"person": {"name": "Joe"}}', E'\"{{&person.name}}\" == \"{{#person}}{{&name}}{{/person}}\"')
UNION
SELECT 21, 'Dotted Names - Arbitrary Depth', mustach('{"a": {"b": {"c": {"d": {"e": {"name": "Phil"}}}}}}', E'\"{{a.b.c.d.e.name}}\" == \"Phil\"')
UNION
SELECT 22, 'Dotted Names - Broken Chains', mustach('{"a": {}}', E'\"{{a.b.c}}\" == \"\"')
UNION
SELECT 23, 'Dotted Names - Broken Chain Resolution', mustach('{"a": {"b": {}}, "c": {"name": "Jim"}}', E'\"{{a.b.c.name}}\" == \"\"')
UNION
SELECT 24, 'Dotted Names - Initial Resolution', mustach('{"a": {"b": {"c": {"d": {"e": {"name": "Phil"}}}}}, "b": {"c": {"d": {"e": {"name": "Wrong"}}}}}', E'\"{{#a}}{{b.c.d.e.name}}{{/a}}\" == \"Phil\"')
UNION
SELECT 25, 'Dotted Names - Context Precedence', mustach('{"a": {"b": {}}, "b": {"c": "ERROR"}}', E'{{#a}}{{b.c}}{{/a}} ')
UNION
SELECT 26, 'Implicit Iterators - Basic Interpolation', mustach('"world"', E'Hello, {{.}}!\n')
UNION
SELECT 27, 'Implicit Iterators - HTML Escaping', mustach('"& \" < >"', E'These characters should be HTML escaped: {{.}}\n')
UNION
SELECT 28, 'Implicit Iterators - Triple Mustache', mustach('"& \" < >"', E'These characters should not be HTML escaped: {{{.}}}\n')
UNION
SELECT 29, 'Implicit Iterators - Ampersand', mustach('"& \" < >"', E'These characters should not be HTML escaped: {{&.}}\n')
UNION
SELECT 30, 'Implicit Iterators - Basic Integer Interpolation', mustach('"85"', E'\"{{.}} miles an hour!\"')
UNION
SELECT 31, 'Interpolation - Surrounding Whitespace', mustach('{"string": "---"}', E'| {{string}} |')
UNION
SELECT 32, 'Triple Mustache - Surrounding Whitespace', mustach('{"string": "---"}', E'| {{{string}}} |')
UNION
SELECT 33, 'Ampersand - Surrounding Whitespace', mustach('{"string": "---"}', E'| {{&string}} |')
UNION
SELECT 34, 'Interpolation - Standalone', mustach('{"string": "---"}', E'  {{string}}\n')
UNION
SELECT 35, 'Triple Mustache - Standalone', mustach('{"string": "---"}', E'  {{{string}}}\n')
UNION
SELECT 36, 'Ampersand - Standalone', mustach('{"string": "---"}', E'  {{&string}}\n')
UNION
SELECT 37, 'Interpolation With Padding', mustach('{"string": "---"}', E'|{{ string }}|')
UNION
SELECT 38, 'Triple Mustache With Padding', mustach('{"string": "---"}', E'|{{{ string }}}|')
UNION
SELECT 39, 'Ampersand With Padding', mustach('{"string": "---"}', E'|{{& string }}|')
ORDER BY 1
) TO stdout;
ROLLBACK;
