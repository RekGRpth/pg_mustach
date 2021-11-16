\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'No Interpolation', mustach('{}', 'Hello from {Mustache}!\n');
SELECT 2, 'Basic Interpolation', mustach('{"subject": "world"}', 'Hello, {{subject}}!\n');
SELECT 3, 'HTML Escaping', mustach('{"forbidden": "& \" < >"}', 'These characters should be HTML escaped: {{forbidden}}\n');
SELECT 4, 'Triple Mustache', mustach('{"forbidden": "& \" < >"}', 'These characters should not be HTML escaped: {{{forbidden}}}\n');
SELECT 5, 'Ampersand', mustach('{"forbidden": "& \" < >"}', 'These characters should not be HTML escaped: {{&forbidden}}\n');
SELECT 6, 'Basic Integer Interpolation', mustach('{"mph": 85}', '\"{{mph}} miles an hour!\"');
SELECT 7, 'Triple Mustache Integer Interpolation', mustach('{"mph": 85}', '\"{{{mph}}} miles an hour!\"');
SELECT 8, 'Ampersand Integer Interpolation', mustach('{"mph": 85}', '\"{{&mph}} miles an hour!\"');
SELECT 9, 'Basic Decimal Interpolation', mustach('{"power": 1.21}', '\"{{power}} jiggawatts!\"');
SELECT 10, 'Triple Mustache Decimal Interpolation', mustach('{"power": 1.21}', '\"{{{power}}} jiggawatts!\"');
SELECT 11, 'Ampersand Decimal Interpolation', mustach('{"power": 1.21}', '\"{{&power}} jiggawatts!\"');
SELECT 12, 'Basic Null Interpolation', mustach('{"cannot": null}', 'I ({{cannot}}) be seen!');
SELECT 13, 'Triple Mustache Null Interpolation', mustach('{"cannot": null}', 'I ({{{cannot}}}) be seen!');
SELECT 14, 'Ampersand Null Interpolation', mustach('{"cannot": null}', 'I ({{&cannot}}) be seen!');
SELECT 15, 'Basic Context Miss Interpolation', mustach('{}', 'I ({{cannot}}) be seen!');
SELECT 16, 'Triple Mustache Context Miss Interpolation', mustach('{}', 'I ({{{cannot}}}) be seen!');
SELECT 17, 'Ampersand Context Miss Interpolation', mustach('{}', 'I ({{&cannot}}) be seen!');
SELECT 18, 'Dotted Names - Basic Interpolation', mustach('{"person": {"name": "Joe"}}', '\"{{person.name}}\" == \"{{#person}}{{name}}{{/person}}\"');
SELECT 19, 'Dotted Names - Triple Mustache Interpolation', mustach('{"person": {"name": "Joe"}}', '\"{{{person.name}}}\" == \"{{#person}}{{{name}}}{{/person}}\"');
SELECT 20, 'Dotted Names - Ampersand Interpolation', mustach('{"person": {"name": "Joe"}}', '\"{{&person.name}}\" == \"{{#person}}{{&name}}{{/person}}\"');
SELECT 21, 'Dotted Names - Arbitrary Depth', mustach('{"a": {"b": {"c": {"d": {"e": {"name": "Phil"}}}}}}', '\"{{a.b.c.d.e.name}}\" == \"Phil\"');
SELECT 22, 'Dotted Names - Broken Chains', mustach('{"a": {}}', '\"{{a.b.c}}\" == \"\"');
SELECT 23, 'Dotted Names - Broken Chain Resolution', mustach('{"a": {"b": {}}, "c": {"name": "Jim"}}', '\"{{a.b.c.name}}\" == \"\"');
SELECT 24, 'Dotted Names - Initial Resolution', mustach('{"a": {"b": {"c": {"d": {"e": {"name": "Phil"}}}}}, "b": {"c": {"d": {"e": {"name": "Wrong"}}}}}', '\"{{#a}}{{b.c.d.e.name}}{{/a}}\" == \"Phil\"');
SELECT 25, 'Dotted Names - Context Precedence', mustach('{"a": {"b": {}}, "b": {"c": "ERROR"}}', '{{#a}}{{b.c}}{{/a}} ');
SELECT 26, 'Implicit Iterators - Basic Interpolation', mustach('"world"', 'Hello, {{.}}!\n');
SELECT 27, 'Implicit Iterators - HTML Escaping', mustach('"& \" < >"', 'These characters should be HTML escaped: {{.}}\n');
SELECT 28, 'Implicit Iterators - Triple Mustache', mustach('"& \" < >"', 'These characters should not be HTML escaped: {{{.}}}\n');
SELECT 29, 'Implicit Iterators - Ampersand', mustach('"& \" < >"', 'These characters should not be HTML escaped: {{&.}}\n');
SELECT 30, 'Implicit Iterators - Basic Integer Interpolation', mustach('"85"', '\"{{.}} miles an hour!\"');
SELECT 31, 'Interpolation - Surrounding Whitespace', mustach('{"string": "---"}', '| {{string}} |');
SELECT 32, 'Triple Mustache - Surrounding Whitespace', mustach('{"string": "---"}', '| {{{string}}} |');
SELECT 33, 'Ampersand - Surrounding Whitespace', mustach('{"string": "---"}', '| {{&string}} |');
SELECT 34, 'Interpolation - Standalone', mustach('{"string": "---"}', '  {{string}}\n');
SELECT 35, 'Triple Mustache - Standalone', mustach('{"string": "---"}', '  {{{string}}}\n');
SELECT 36, 'Ampersand - Standalone', mustach('{"string": "---"}', '  {{&string}}\n');
SELECT 37, 'Interpolation With Padding', mustach('{"string": "---"}', '|{{ string }}|');
SELECT 38, 'Triple Mustache With Padding', mustach('{"string": "---"}', '|{{{ string }}}|');
SELECT 39, 'Ampersand With Padding', mustach('{"string": "---"}', '|{{& string }}|');
ROLLBACK;
