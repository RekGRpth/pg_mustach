\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'string equal exact match', mustach('{"s":"abc"}', '{{#s=abc}}yes{{/s=abc}}');
SELECT 2, 'string equal, matching prefix but different length', mustach('{"s":"abc"}', '{{#s=abcd}}yes{{/s=abcd}}');
SELECT 3, 'string less-than by real content divergence', mustach('{"s":"abc"}', '{{#s<abd}}yes{{/s<abd}}');
SELECT 4, 'string less-or-equal on exact match', mustach('{"s":"abc"}', '{{#s<=abc}}yes{{/s<=abc}}');
SELECT 5, 'string less-than by length tie-break, stored value shorter', mustach('{"s":"ab"}', '{{#s<abc}}yes{{/s<abc}}');
SELECT 6, 'string greater-than by length tie-break, stored value longer', mustach('{"s":"abcd"}', '{{#s>abc}}yes{{/s>abc}}');
SELECT 7, 'empty stored string still fails the truthiness gate even when the compare matches', mustach('{"s":""}', '{{#s<x}}yes{{/s<x}}');
SELECT 8, 'bool true equal true renders (compare and truthiness both pass)', mustach('{"b":true}', '{{#b=true}}yes{{/b=true}}');
SELECT 9, 'bool false equal false does not render (compare matches but false is never truthy)', mustach('{"b":false}', '{{#b=false}}yes{{/b=false}}');
SELECT 10, 'null equal null does not render (compare matches but null is never truthy)', mustach('{"n":null}', '{{#n=null}}yes{{/n=null}}');
SELECT 11, 'null not equal to an arbitrary literal', mustach('{"n":null}', '{{#n=abc}}yes{{/n=abc}}');
SELECT 12, 'container compared with greater-than always reports greater (default branch)', mustach('{"o":{"x":1}}', '{{#o>abc}}yes{{/o>abc}}');
SELECT 13, 'container compared with less-than never reports lesser', mustach('{"o":{"x":1}}', '{{#o<abc}}yes{{/o<abc}}');
ROLLBACK;
