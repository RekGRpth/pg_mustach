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
SELECT 1, 'Basic Behavior', mustach('{"text": "from partial"}', E'\"{{>text}}\"')
UNION
SELECT 2, 'Failed Lookup', mustach('{}', E'\"{{>text}}\"')
UNION
SELECT 3, 'Context', mustach('{"text": "content", "partial": "*{{text}}*"}', E'\"{{>partial}}\"')
UNION
SELECT 4, 'Recursion', mustach('{"content": "X", "nodes": [{"content": "Y", "nodes": []}], "node": "{{content}}<{{#nodes}}{{>node}}{{/nodes}}>"}', E'{{>node}}')
UNION
SELECT 5, 'Surrounding Whitespace', mustach('{"partial": "\t|\t"}', E'| {{>partial}} |')
UNION
SELECT 6, 'Inline Indentation', mustach('{"data": "|", "partial": ">\n>"}', E'  {{data}}  {{> partial}}\n')
UNION
SELECT 7, 'Standalone Line Endings', mustach('{"partial": ">"}', E'|\r\n{{>partial}}\r\n|')
UNION
SELECT 8, 'Standalone Without Previous Line', mustach('{"partial": ">\n>"}', E'  {{>partial}}\n>')
UNION
SELECT 9, 'Standalone Without Newline', mustach('{"partial": ">\n>"}', E'>\n  {{>partial}}')
UNION
SELECT 10, 'Standalone Indentation', mustach('{"content": "<\n->", "partial": "|\n{{{content}}}\n|\n"}', E'\\\n {{>partial}}\n/\n')
UNION
SELECT 11, 'Padding Whitespace', mustach('{"boolean": true, "partial": "[]"}', E'|{{> partial }}|')
ORDER BY 1
) TO stdout;
ROLLBACK;
