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
SELECT 14, 'numeric equal is exact beyond double precision', mustach('{"n":12345678901234567890}', '{{#n=12345678901234567891}}yes{{/n=12345678901234567891}}');
SELECT 15, 'numeric equal on exact match beyond double precision', mustach('{"n":12345678901234567890}', '{{#n=12345678901234567890}}yes{{/n=12345678901234567890}}');
SELECT 16, 'numeric equal is exact for decimals double would round together', mustach('{"n":0.30000000000000001}', '{{#n=0.3}}yes{{/n=0.3}}');
SELECT 17, 'numeric greater-than is exact for decimals double would round together', mustach('{"n":10}', '{{#n>9.99999999999999999}}yes{{/n>9.99999999999999999}}');
SELECT 18, 'numeric equal across notations', mustach('{"n":150}', '{{#n=1.50e2}}yes{{/n=1.50e2}}');
SELECT 19, 'numeric less-than with negative decimals', mustach('{"n":-5}', '{{#n<-4.5}}yes{{/n<-4.5}}');
SELECT 20, 'numeric compared with a non-number keeps comparing against atof() of it', mustach('{"n":5}', '{{#n>abc}}yes{{/n>abc}}');
SELECT 21, 'numeric compared with hex keeps comparing against atof() of it on every version', mustach('{"n":16}', '{{#n=0x10}}yes{{/n=0x10}}');
SELECT 22, 'zero equal zero does not render (compare matches but zero is never truthy)', mustach('{"n":0}', '{{#n=0}}yes{{/n=0}}');
ROLLBACK;
