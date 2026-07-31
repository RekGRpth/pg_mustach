\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'superuser can write new file', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_allowed.txt');
CREATE ROLE mustach_test_nonpriv NOSUPERUSER;
SET LOCAL ROLE mustach_test_nonpriv;
\set ON_ERROR_STOP false
SELECT 2, 'non-superuser cannot write file', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_denied.txt');
\set ON_ERROR_STOP true
RESET ROLE;
\set ON_ERROR_STOP false
SELECT 3, 'cannot overwrite existing file', mustach('{"a":"c"}', '{{a}}', '/tmp/pg_mustach_test_allowed.txt');
\set ON_ERROR_STOP true
SELECT 4, 'existing file content untouched', pg_read_file('/tmp/pg_mustach_test_allowed.txt');
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt
