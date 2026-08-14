\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
\! rm -f /tmp/pg_mustach_test_prepare.txt /tmp/pg_mustach_test_prepare_positional_footgun.txt
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT mustach_template('{{a}}');
SELECT 1, 'render with no tplname uses the unnamed default slot', mustach_json('{"a":"unnamed"}');
SELECT mustach_template('<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>', 'people');
SELECT 2, 'named template renders one jsonb value', mustach_json('{"people":[{"firstName":"Yehuda","lastName":"Katz"}]}', tplname := 'people');
SELECT 3, 'same named template, second jsonb value', mustach_json('{"people":[{"firstName":"Carl","lastName":"Lerche"},{"firstName":"Alan","lastName":"Johnson"}]}', tplname := 'people');
SELECT mustach_template('{{b}}', 'people');
SELECT 4, 're-preparing an existing tplname replaces it rather than leaking the old one', mustach_json('{"a":"stale","b":"fresh"}', tplname := 'people');
SELECT 5, 'forget removes a known tplname', mustach_forget('people');
SELECT 6, 'forget on an already-unknown tplname returns false', mustach_forget('people');
\set ON_ERROR_STOP false
SELECT 7, 'render on a forgotten tplname errors', mustach_json('{}', tplname := 'people');
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 8, 'render on a tplname that was never prepared errors', mustach_json('{}', tplname := 'doesnotexist');
\set ON_ERROR_STOP true
SELECT mustach_template('{{a}}', 'file_test');
SELECT 9, 'render can write the result to a server file instead of returning it', mustach_json('{"a":"b"}', '/tmp/pg_mustach_test_prepare.txt', 'file_test');
\! printf '10|file content written by render|%s\n' "$(cat /tmp/pg_mustach_test_prepare.txt)"
CREATE ROLE mustach_json_test_nonpriv NOSUPERUSER;
SET LOCAL ROLE mustach_json_test_nonpriv;
\set ON_ERROR_STOP false
SELECT 11, 'non-superuser cannot write file via render', mustach_json('{"a":"b"}', '/tmp/pg_mustach_test_prepare_denied.txt', 'file_test');
\set ON_ERROR_STOP true
RESET ROLE;
--
-- tplname has DEFAULT NULL on both mustach_json overloads, and a bare
-- positional 2nd argument is an "unknown"-type string literal that
-- PostgreSQL's overload resolution prefers to bind to a TEXT parameter
-- over a NAME parameter -- so a positional call actually picks the
-- (json, file, tplname DEFAULT NULL) overload, not (json, tplname), and
-- silently writes a server file rather than rendering by name. This is
-- locked in here so it isn't "fixed" by accident: always call with
-- tplname := '...' instead (see the tests above).
--
SELECT 12, 'a bare positional 2nd argument is the file overload, not tplname', mustach_json('{"a":"b"}', '/tmp/pg_mustach_test_prepare_positional_footgun.txt');
\! test -e /tmp/pg_mustach_test_prepare_positional_footgun.txt && echo '13|positional call wrote a file|yes' || echo '13|positional call wrote a file|no'
SELECT 14, 'forget on the unnamed default slot returns true once', mustach_forget();
SELECT 15, 'forget on the already-empty unnamed default slot returns false', mustach_forget();
\set ON_ERROR_STOP false
SELECT 16, 'render with no tplname errors once the default slot is empty', mustach_json('{}');
\set ON_ERROR_STOP true
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_prepare.txt /tmp/pg_mustach_test_prepare_denied.txt /tmp/pg_mustach_test_prepare_positional_footgun.txt
