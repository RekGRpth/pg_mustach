\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt /tmp/pg_mustach_test_debris.txt /tmp/pg_mustach_test_abort.txt /tmp/pg_mustach_test_fd*.txt
CREATE EXTENSION pg_mustach;
SELECT 1, 'superuser can write new file', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_allowed.txt');
CREATE ROLE mustach_test_nonpriv NOSUPERUSER;
SET ROLE mustach_test_nonpriv;
SELECT 2, 'non-superuser cannot write file', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_denied.txt');
RESET ROLE;
SELECT 3, 'cannot overwrite existing file', mustach('{"a":"c"}', '{{a}}', '/tmp/pg_mustach_test_allowed.txt');
\! printf '4|existing file content untouched|%s\n' "$(cat /tmp/pg_mustach_test_allowed.txt)"
SELECT 5, 'failing render does not create debris file', mustach('{"a":"b"}', '{{#unclosed}}', '/tmp/pg_mustach_test_debris.txt');
\! test -e /tmp/pg_mustach_test_debris.txt && echo '6|debris file left behind|yes' || echo '6|debris file left behind|no'
SELECT 7, 'retry with fixed template succeeds', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_debris.txt');
SET pg_mustach.whitelist = 'file:///nonexistent/';
SELECT 8, 'ERROR raised mid-render does not create debris file', mustach('{}', 'x{{>/etc/hostname}}', '/tmp/pg_mustach_test_abort.txt');
\! test -e /tmp/pg_mustach_test_abort.txt && echo '9|debris file left behind|yes' || echo '9|debris file left behind|no'
SET pg_mustach.transaction = false;
SELECT 10, 'prepare template with denied partial', mustach_template('x{{>/etc/hostname}}') IS NOT NULL;
SELECT 11, 'ERROR raised mid-render of prepared template does not create debris file', mustach_json_file('{}', '/tmp/pg_mustach_test_abort.txt');
\! test -e /tmp/pg_mustach_test_abort.txt && echo '12|debris file left behind|yes' || echo '12|debris file left behind|no'
DO $$ BEGIN
    FOR i IN 1..5 LOOP
        BEGIN PERFORM mustach('{}', 'x{{>/etc/hostname}}', '/tmp/pg_mustach_test_fd' || i || 'a.txt'); EXCEPTION WHEN insufficient_privilege THEN NULL; END;
        BEGIN PERFORM mustach_json_file('{}', '/tmp/pg_mustach_test_fd' || i || 'b.txt'); EXCEPTION WHEN insufficient_privilege THEN NULL; END;
    END LOOP;
END $$;
CREATE TEMP TABLE pg_mustach_test_fds (n int);
COPY pg_mustach_test_fds FROM PROGRAM 'ls -l /proc/$PPID/fd | grep -c pg_mustach_test_fd || true';
SELECT 13, 'fds left open by repeated ERRORs', n FROM pg_mustach_test_fds;
\! printf '14|debris files left behind by repeated ERRORs|%s\n' "$(ls /tmp/pg_mustach_test_fd*.txt 2>/dev/null | wc -l)"
RESET pg_mustach.whitelist;
RESET pg_mustach.transaction;
SELECT 15, 'retry with same path succeeds', mustach('{"a":"b"}', '{{a}}', '/tmp/pg_mustach_test_abort.txt');
DROP ROLE mustach_test_nonpriv;
DROP EXTENSION pg_mustach;
\! rm -f /tmp/pg_mustach_test_allowed.txt /tmp/pg_mustach_test_denied.txt /tmp/pg_mustach_test_debris.txt /tmp/pg_mustach_test_abort.txt /tmp/pg_mustach_test_fd*.txt
