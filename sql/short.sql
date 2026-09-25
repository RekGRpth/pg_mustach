\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'content json', mustach('{"a":"b"}', '{{a}}');
SELECT 2, 'content json', mustach('{"people":[{"firstName":"Yehuda","lastName":"Katz"},{"firstName":"Carl","lastName":"Lerche"},{"firstName":"Alan","lastName":"Johnson"}]}', '<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>');
SELECT 3, 'an empty template renders empty, not whatever follows it in memory', '[' || mustach('{"a":"b"}', '') || ']';
\! rm -f /tmp/pg_mustach_test_empty.txt
SELECT 4, 'an empty template writes an empty file', mustach('{"a":"b"}', '', '/tmp/pg_mustach_test_empty.txt');
\! printf '5|empty template file size|%s\n' "$(wc -c < /tmp/pg_mustach_test_empty.txt)"; rm -f /tmp/pg_mustach_test_empty.txt
ROLLBACK;
