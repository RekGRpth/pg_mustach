\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt /tmp/pg_mustach_test_debris.txt /tmp/pg_mustach_test_abort.txt /tmp/pg_mustach_test_fd*.txt
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
\! printf '4|existing file content untouched|%s\n' "$(cat /tmp/pg_mustach_test_allowed.txt)"
\set ON_ERROR_STOP false
SELECT 5, 'failing render does not create debris file', mustach('{"a":"b"}', '{{#unclosed}}', '/tmp/pg_mustach_test_debris.txt');
\set ON_ERROR_STOP true
\! test -e /tmp/pg_mustach_test_debris.txt && echo '6|debris file left behind|yes' || echo '6|debris file left behind|no'
SELECT 7, 'retry with fixed template succeeds', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_debris.txt');
SET LOCAL pg_mustach.whitelist = 'file:///nonexistent/';
\set ON_ERROR_STOP false
SELECT 8, 'ERROR raised mid-render does not create debris file', mustach('{}', 'x{{>/etc/hostname}}', '/tmp/pg_mustach_test_abort.txt');
\set ON_ERROR_STOP true
\! test -e /tmp/pg_mustach_test_abort.txt && echo '9|debris file left behind|yes' || echo '9|debris file left behind|no'
SELECT 10, 'prepare template with denied partial', mustach_template('x{{>/etc/hostname}}') IS NOT NULL;
\set ON_ERROR_STOP false
SELECT 11, 'ERROR raised mid-render of prepared template does not create debris file', mustach_json_file('{}', '/tmp/pg_mustach_test_abort.txt');
\set ON_ERROR_STOP true
\! test -e /tmp/pg_mustach_test_abort.txt && echo '12|debris file left behind|yes' || echo '12|debris file left behind|no'
SELECT count(*) AS fd_before FROM pg_ls_dir('/proc/self/fd') \gset
DO $$ BEGIN
    FOR i IN 1..5 LOOP
        BEGIN PERFORM mustach('{}', 'x{{>/etc/hostname}}', '/tmp/pg_mustach_test_fd' || i || 'a.txt'); EXCEPTION WHEN insufficient_privilege THEN NULL; END;
        BEGIN PERFORM mustach_json_file('{}', '/tmp/pg_mustach_test_fd' || i || 'b.txt'); EXCEPTION WHEN insufficient_privilege THEN NULL; END;
    END LOOP;
END $$;
SELECT 13, 'repeated ERRORs mid-render leak no fds', count(*) = :fd_before FROM pg_ls_dir('/proc/self/fd');
\! printf '14|debris files left behind by repeated ERRORs|%s\n' "$(ls /tmp/pg_mustach_test_fd*.txt 2>/dev/null | wc -l)"
RESET pg_mustach.whitelist;
SELECT 15, 'retry with same path succeeds', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_abort.txt');
ROLLBACK;
\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt /tmp/pg_mustach_test_debris.txt /tmp/pg_mustach_test_abort.txt /tmp/pg_mustach_test_fd*.txt
