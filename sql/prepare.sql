\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
\! rm -f /tmp/pg_mustach_test_prepare.txt
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'prepare returns a handle' FROM (SELECT mustach_prepare('{{a}}') AS id) s WHERE id IS NOT NULL;
WITH prepared AS (SELECT mustach_prepare('<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>') AS id)
SELECT 2, 'one prepared template renders many different jsonb values', mustach_render(id, '{"people":[{"firstName":"Yehuda","lastName":"Katz"}]}') FROM prepared;
WITH prepared AS (SELECT mustach_prepare('<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>') AS id)
SELECT 3, 'same prepared template, second jsonb value', mustach_render(id, '{"people":[{"firstName":"Carl","lastName":"Lerche"},{"firstName":"Alan","lastName":"Johnson"}]}') FROM prepared;
SELECT 4, 'forget removes a known handle', mustach_forget(mustach_prepare('{{a}}'));
\set ON_ERROR_STOP false
SELECT 5, 'render on an unknown handle errors', mustach_render(-1, '{}');
\set ON_ERROR_STOP true
SELECT 6, 'forget on an already-unknown handle returns false', mustach_forget(-1);
WITH prepared AS (SELECT mustach_prepare('{{a}}') AS id)
SELECT 7, 'render can write the result to a server file instead of returning it', mustach_render(id, '{"a":"b"}', '/tmp/pg_mustach_test_prepare.txt') FROM prepared;
\! printf '8|file content written by render|%s\n' "$(cat /tmp/pg_mustach_test_prepare.txt)"
CREATE ROLE mustach_render_test_nonpriv NOSUPERUSER;
SET LOCAL ROLE mustach_render_test_nonpriv;
\set ON_ERROR_STOP false
WITH prepared AS (SELECT mustach_prepare('{{a}}') AS id)
SELECT 9, 'non-superuser cannot write file via render', mustach_render(id, '{"a":"b"}', '/tmp/pg_mustach_test_prepare_denied.txt') FROM prepared;
\set ON_ERROR_STOP true
RESET ROLE;
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_prepare.txt /tmp/pg_mustach_test_prepare_denied.txt
