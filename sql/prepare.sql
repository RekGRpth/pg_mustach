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
SELECT 5, 'forget removes a known tplname', mustach_free('people');
SELECT 6, 'forget on an already-unknown tplname returns false', mustach_free('people');
\set ON_ERROR_STOP false
SELECT 7, 'render on a forgotten tplname errors', mustach_json('{}', tplname := 'people');
\set ON_ERROR_STOP true
\set ON_ERROR_STOP false
SELECT 8, 'render on a tplname that was never prepared errors', mustach_json('{}', tplname := 'doesnotexist');
\set ON_ERROR_STOP true
SELECT mustach_template('{{a}}', 'file_test');
SELECT 9, 'render can write the result to a server file instead of returning it', mustach_json_file('{"a":"b"}', '/tmp/pg_mustach_test_prepare.txt', 'file_test');
\! printf '10|file content written by render|%s\n' "$(cat /tmp/pg_mustach_test_prepare.txt)"
CREATE ROLE mustach_json_test_nonpriv NOSUPERUSER;
SET LOCAL ROLE mustach_json_test_nonpriv;
\set ON_ERROR_STOP false
SELECT 11, 'non-superuser cannot write file via render', mustach_json_file('{"a":"b"}', '/tmp/pg_mustach_test_prepare_denied.txt', 'file_test');
\set ON_ERROR_STOP true
RESET ROLE;
--
-- Writing to a file is mustach_json_file(), not a mustach_json() overload:
-- with (json, file text, tplname DEFAULT NULL) as a second mustach_json(),
-- overload resolution bound a bare positional 2nd argument (an unknown-type
-- literal, which prefers TEXT over NAME) to file, so mustach_json(json,
-- 'people') silently wrote a server file named "people" instead of
-- rendering that template. Now it can only ever be tplname, and a path
-- passed there is just an unknown template name -- nothing gets written.
--
SELECT 12, 'a bare positional 2nd argument is tplname', mustach_json('{"a":"b"}', 'file_test');
\set ON_ERROR_STOP false
SELECT 13, 'a path as a bare positional 2nd argument is just an unknown tplname', mustach_json('{"a":"b"}', '/tmp/pg_mustach_test_prepare_positional_footgun.txt');
\set ON_ERROR_STOP true
\! test -e /tmp/pg_mustach_test_prepare_positional_footgun.txt && echo '13|positional call wrote a file|yes' || echo '13|positional call wrote a file|no'
SELECT 14, 'forget on the unnamed default slot returns true once', mustach_free();
SELECT 15, 'forget on the already-empty unnamed default slot returns false', mustach_free();
\set ON_ERROR_STOP false
SELECT 16, 'render with no tplname errors once the default slot is empty', mustach_json('{}');
\set ON_ERROR_STOP true
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_prepare.txt /tmp/pg_mustach_test_prepare_denied.txt /tmp/pg_mustach_test_prepare_positional_footgun.txt
