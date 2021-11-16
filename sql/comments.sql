\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'Inline', replace(mustach('{}', E'12345{{! Comment Block! }}67890'), E'\n', '\n');
SELECT 2, 'Multiline', replace(mustach('{}', E'12345{{!\n  This is a\n  multi-line comment...\n}}67890\n'), E'\n', '\n');
SELECT 3, 'Standalone', replace(mustach('{}', E'Begin.\n{{! Comment Block! }}\nEnd.\n'), E'\n', '\n');
SELECT 4, 'Indented Standalone', replace(mustach('{}', E'Begin.\n  {{! Indented Comment Block! }}\nEnd.\n'), E'\n', '\n');
SELECT 5, 'Standalone Line Endings', replace(replace(mustach('{}', E'|\r\n{{! Standalone Comment }}\r\n|'), E'\r', '\r'), E'\n', '\n');
SELECT 6, 'Standalone Without Previous Line', replace(mustach('{}', E'  {{! I''m Still Standalone }}\n!'), E'\n', '\n');
SELECT 7, 'Standalone Without Newline', replace(mustach('{}', E'!\n  {{! I''m Still Standalone }}'), E'\n', '\n');
SELECT 8, 'Multiline Standalone', replace(mustach('{}', E'Begin.\n{{!\nSomething''s going on here...\n}}\nEnd.\n'), E'\n', '\n');
SELECT 9, 'Indented Multiline Standalone', replace(mustach('{}', E'Begin.\n  {{!\n    Something''s going on here...\n  }}\nEnd.\n'), E'\n', '\n');
SELECT 10, 'Indented Inline', replace(mustach('{}', E'  12 {{! 34 }}\n'), E'\n', '\n');
SELECT 11, 'Surrounding Whitespace', replace(mustach('{}', E'12345 {{! Comment Block! }} 67890'), E'\n', '\n');
ROLLBACK;
