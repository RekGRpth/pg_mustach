\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'Pair Behavior', mustach('{"text": "Hey!"}', '{{=<% %>=}}(<%text%>)');
SELECT 2, 'Special Characters', mustach('{"text": "It worked!"}', '({{=[ ]=}}[text])');
SELECT 3, 'Special Characters', mustach('{"section": true, "data": "I got interpolated."}', '[\n{{#section}}\n  {{data}}\n  |data|\n{{/section}}\n\n{{=| |=}}\n|#section|\n  {{data}}\n  |data|\n|/section|\n]\n');
SELECT 4, 'Inverted Sections', mustach('{"section": false, "data": "I got interpolated."}', '[\n{{^section}}\n  {{data}}\n  |data|\n{{/section}}\n\n{{=| |=}}\n|^section|\n  {{data}}\n  |data|\n|/section|\n]\n');
SELECT 5, 'Partial Inheritence', mustach('{"value": "yes", "include": ".{{value}}."}', '[ {{>include}} ]\n{{=| |=}}\n[ |>include| ]\n');
SELECT 6, 'Post-Partial Behavior', mustach('{"value": "yes", "include": ".{{value}}. {{=| |=}} .|value|."}', '[ {{>include}} ]\n[ .{{value}}.  .|value|. ]\n');
SELECT 7, 'Surrounding Whitespace', mustach('{}', '| {{=@ @=}} |');
SELECT 8, 'Outlying Whitespace (Inline)', mustach('{}', ' | {{=@ @=}}\n');
SELECT 9, 'Standalone Tag', mustach('{}', 'Begin.\n  {{=@ @=}}\nEnd.\n');
SELECT 10, 'Indented Standalone Tag', mustach('{}', 'Begin.\n  {{=@ @=}}\nEnd.\n');
SELECT 11, 'Standalone Line Endings', mustach('{}', '|\r\n{{=@ @=}}\r\n|');
SELECT 12, 'Standalone Without Previous Line', mustach('{}', '  {{=@ @=}}\n=');
SELECT 13, 'Standalone Without Newline', mustach('{}', '=\n  {{=@ @=}}');
SELECT 14, 'Pair with Padding', mustach('{}', '|{{=@   @=}}|');
ROLLBACK;
