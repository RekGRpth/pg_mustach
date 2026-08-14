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
SELECT mustach_prepare('{{a}}');
SELECT 1, 'render with no tplname uses the unnamed default slot', mustach_render('{"a":"unnamed"}');
SELECT mustach_prepare('<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>', 'people');
SELECT 2, 'named template renders one jsonb value', mustach_render('{"people":[{"firstName":"Yehuda","lastName":"Katz"}]}', 'people');
SELECT 3, 'same named template, second jsonb value', mustach_render('{"people":[{"firstName":"Carl","lastName":"Lerche"},{"firstName":"Alan","lastName":"Johnson"}]}', 'people');
SELECT mustach_prepare('{{b}}', 'people');
SELECT 4, 're-preparing an existing tplname replaces it rather than leaking the old one', mustach_render('{"a":"stale","b":"fresh"}', 'people');
SELECT 5, 'forget removes a known tplname', mustach_forget('people');
SELECT 6, 'forget on an already-unknown tplname returns false', mustach_forget('people');
\set ON_ERROR_STOP false
SELECT 7, 'render on a forgotten tplname errors', mustach_render('{}', 'people');
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 8, 'render on a tplname that was never prepared errors', mustach_render('{}', 'doesnotexist');
\set ON_ERROR_STOP true
SELECT mustach_prepare('{{a}}', 'file_test');
SELECT 9, 'render can write the result to a server file instead of returning it', mustach_render('{"a":"b"}', '/tmp/pg_mustach_test_prepare.txt', 'file_test');
\! printf '10|file content written by render|%s\n' "$(cat /tmp/pg_mustach_test_prepare.txt)"
CREATE ROLE mustach_render_test_nonpriv NOSUPERUSER;
SET LOCAL ROLE mustach_render_test_nonpriv;
\set ON_ERROR_STOP false
SELECT 11, 'non-superuser cannot write file via render', mustach_render('{"a":"b"}', '/tmp/pg_mustach_test_prepare_denied.txt', 'file_test');
\set ON_ERROR_STOP true
RESET ROLE;
SELECT 12, 'forget on the unnamed default slot returns true once', mustach_forget();
SELECT 13, 'forget on the already-empty unnamed default slot returns false', mustach_forget();
\set ON_ERROR_STOP false
SELECT 14, 'render with no tplname errors once the default slot is empty', mustach_render('{}');
\set ON_ERROR_STOP true
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_prepare.txt /tmp/pg_mustach_test_prepare_denied.txt
