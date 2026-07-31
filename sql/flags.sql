\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach VERSION '1.0';
ALTER EXTENSION pg_mustach UPDATE TO '2.0';
SELECT 1, 'colon flag value', mustach_with_colon();
SELECT 2, 'compare flag value', mustach_with_compare();
SELECT 3, 'noextensions value', mustach_with_noextensions();
SELECT 4, 'allextensions value', mustach_with_allextensions();
SELECT 5, 'default flags GUC', current_setting('pg_mustach.flags');
SELECT 6, 'colon extension enabled by default', mustach('{"a":"b"}', '{{:a}}');
SELECT mustach_set_flags(mustach_with_noextensions());
SELECT 7, 'colon extension disabled', mustach('{"a":"b"}', '{{:a}}');
SAVEPOINT sp1;
SELECT mustach_set_flags(mustach_with_colon() | mustach_with_compare(), true);
SELECT 8, 'colon extension re-enabled in subtransaction', mustach('{"a":"b"}', '{{:a}}');
SELECT 9, 'combined flags in subtransaction', current_setting('pg_mustach.flags');
ROLLBACK TO SAVEPOINT sp1;
SELECT 10, 'flags reverted after rollback to savepoint', mustach('{"a":"b"}', '{{:a}}');
ROLLBACK;
SELECT 11, 'flags reverted after outer rollback', current_setting('pg_mustach.flags');
