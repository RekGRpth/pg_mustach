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
SELECT 1, 'Inline', mustach('{}', E'12345{{! Comment Block! }}67890')
UNION
SELECT 2, 'Multiline', mustach('{}', E'12345{{!\n  This is a\n  multi-line comment...\n}}67890\n')
UNION
SELECT 3, 'Standalone', mustach('{}', E'Begin.\n{{! Comment Block! }}\nEnd.\n')
UNION
SELECT 4, 'Indented Standalone', mustach('{}', E'Begin.\n  {{! Indented Comment Block! }}\nEnd.\n')
UNION
SELECT 5, 'Standalone Line Endings', mustach('{}', E'|\r\n{{! Standalone Comment }}\r\n|')
UNION
SELECT 6, 'Standalone Without Previous Line', mustach('{}', E'  {{! I''m Still Standalone }}\n!')
UNION
SELECT 7, 'Standalone Without Newline', mustach('{}', E'!\n  {{! I''m Still Standalone }}')
UNION
SELECT 8, 'Multiline Standalone', mustach('{}', E'Begin.\n{{!\nSomething''s going on here...\n}}\nEnd.\n')
UNION
SELECT 9, 'Indented Multiline Standalone', mustach('{}', E'Begin.\n  {{!\n    Something''s going on here...\n  }}\nEnd.\n')
UNION
SELECT 10, 'Indented Inline', mustach('{}', E'  12 {{! 34 }}\n')
UNION
SELECT 11, 'Surrounding Whitespace', mustach('{}', E'12345 {{! Comment Block! }} 67890')
ORDER BY 1
) TO stdout;
ROLLBACK;
