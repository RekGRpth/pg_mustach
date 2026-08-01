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
SELECT 1, 'Overflowing double is truthy', mustach('{"n": 1e400}', E'\"{{#n}}yes{{/n}}\"')
UNION
SELECT 2, 'Overflowing double is not falsey', mustach('{"n": 1e400}', E'\"{{^n}}no{{/n}}\"')
UNION
SELECT 3, 'Overflowing double keeps exact precision', mustach('{"n": 1e400}', E'\"{{n}}\"')
UNION
SELECT 4, 'Underflowing double is truthy', mustach('{"n": 1e-400}', E'\"{{#n}}yes{{/n}}\"')
UNION
SELECT 5, 'Underflowing double is not falsey', mustach('{"n": 1e-400}', E'\"{{^n}}no{{/n}}\"')
UNION
SELECT 6, 'Zero is falsey', mustach('{"n": 0}', E'\"{{#n}}yes{{/n}}\"')
UNION
SELECT 7, 'Zero triggers inverted section', mustach('{"n": 0}', E'\"{{^n}}no{{/n}}\"')
UNION
SELECT 8, 'Negative zero is falsey', mustach('{"n": -0.0}', E'\"{{#n}}yes{{/n}}\"')
UNION
SELECT 9, 'Compare against overflowing double does not error', mustach('{"n": 1e400}', E'\"{{#n=5}}eq{{/n=5}}\"')
UNION
SELECT 10, 'Ordered compare against overflowing double does not error', mustach('{"n": 1e400}', E'\"{{#n<10}}lt{{/n<10}}\"')
ORDER BY 1
) TO stdout;
ROLLBACK;
