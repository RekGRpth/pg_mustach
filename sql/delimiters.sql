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
SELECT 1, 'Pair Behavior', mustach('{"text": "Hey!"}', E'{{=<% %>=}}(<%text%>)')
UNION
SELECT 2, 'Special Characters', mustach('{"text": "It worked!"}', E'({{=[ ]=}}[text])')
UNION
SELECT 3, 'Special Characters', mustach('{"section": true, "data": "I got interpolated."}', E'[\n{{#section}}\n  {{data}}\n  |data|\n{{/section}}\n\n{{=| |=}}\n|#section|\n  {{data}}\n  |data|\n|/section|\n]\n')
UNION
SELECT 4, 'Inverted Sections', mustach('{"section": false, "data": "I got interpolated."}', E'[\n{{^section}}\n  {{data}}\n  |data|\n{{/section}}\n\n{{=| |=}}\n|^section|\n  {{data}}\n  |data|\n|/section|\n]\n')
UNION
SELECT 5, 'Partial Inheritence', mustach('{"value": "yes", "include": ".{{value}}."}', E'[ {{>include}} ]\n{{=| |=}}\n[ |>include| ]\n')
UNION
SELECT 6, 'Post-Partial Behavior', mustach('{"value": "yes", "include": ".{{value}}. {{=| |=}} .|value|."}', E'[ {{>include}} ]\n[ .{{value}}.  .|value|. ]\n')
UNION
SELECT 7, 'Surrounding Whitespace', mustach('{}', E'| {{=@ @=}} |')
UNION
SELECT 8, 'Outlying Whitespace (Inline)', mustach('{}', E' | {{=@ @=}}\n')
UNION
SELECT 9, 'Standalone Tag', mustach('{}', E'Begin.\n  {{=@ @=}}\nEnd.\n')
UNION
SELECT 10, 'Indented Standalone Tag', mustach('{}', E'Begin.\n  {{=@ @=}}\nEnd.\n')
UNION
SELECT 11, 'Standalone Line Endings', mustach('{}', E'|\r\n{{=@ @=}}\r\n|')
UNION
SELECT 12, 'Standalone Without Previous Line', mustach('{}', E'  {{=@ @=}}\n=')
UNION
SELECT 13, 'Standalone Without Newline', mustach('{}', E'=\n  {{=@ @=}}')
UNION
SELECT 14, 'Pair with Padding', mustach('{}', E'|{{=@   @=}}|')
ORDER BY 1
) TO stdout;
ROLLBACK;
