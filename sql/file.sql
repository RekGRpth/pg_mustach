\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'superuser can write file', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_allowed.txt');
CREATE ROLE mustach_test_nonpriv NOSUPERUSER;
SET LOCAL ROLE mustach_test_nonpriv;
\set ON_ERROR_STOP false
SELECT 2, 'non-superuser cannot write file', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_denied.txt');
\set ON_ERROR_STOP true
RESET ROLE;
SELECT 3, 'seed file with content', mustach('{"a":"seeded"}', '{{a}}', '/tmp/pg_mustach_test_truncation.txt');
\set ON_ERROR_STOP false
SELECT 4, 'failing render must not truncate existing file', mustach('{"a":"b"}', '{{#unclosed}}', '/tmp/pg_mustach_test_truncation.txt');
\set ON_ERROR_STOP true
SELECT 5, 'file content survived failed render', pg_read_file('/tmp/pg_mustach_test_truncation.txt');
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt /tmp/pg_mustach_test_truncation.txt
