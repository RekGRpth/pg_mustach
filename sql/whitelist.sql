CREATE EXTENSION pg_mustach;

-- Two roles: an ordinary non-superuser, and a superuser -- pg_mustach's C
-- code (pg_mustach_privileged()) checks superuser() directly, nothing more
-- granular. Role names can't start with "pg_" (reserved), so use
-- mustach_test_*.
--
-- Both roles need LOGIN: the pg_mustach.whitelist tests below need to \c
-- into them directly, since ALTER ROLE ... SET only takes effect for a new
-- connection as that role, not retroactively via SET ROLE in an
-- already-open session.
CREATE ROLE mustach_test_none LOGIN;
CREATE ROLE mustach_test_full LOGIN SUPERUSER;

--
-- {{>name}} only falls back to reading "name" as a local file when "name"
-- isn't found in the json data -- confirm ordinary partial resolution (and
-- an undefined partial, which silently renders empty rather than erroring)
-- is unaffected by any of this, before the file-loading fallback below is
-- ever reached.
--
SELECT mustach('{"text": "from partial"}', E'"{{>text}}"');
SELECT mustach('{}', E'"{{>nonexistent_partial_name}}"');

--
-- Without superuser and without a pg_mustach.whitelist, the file-loading
-- fallback must be denied.
--
SET ROLE mustach_test_none;
SELECT mustach('{}', E'{{>/etc/hostname}}');
RESET ROLE;

--
-- pg_mustach.whitelist can grant mustach_test_none access despite it not
-- being a superuser -- an explicit entry becomes its sole authorization for
-- that specific file, rather than merely narrowing already-privileged
-- access the way it does for mustach_test_full further down. Needs \c, like
-- the mustach_test_full whitelist tests further down, since ALTER ROLE ...
-- SET doesn't apply retroactively via SET ROLE in this already-open
-- session.
--
SELECT current_user AS pg_mustach_test_orig_user \gset
ALTER ROLE mustach_test_none SET pg_mustach.whitelist = 'file:///etc/hostname';
\c - mustach_test_none

SELECT octet_length(mustach('{}', E'{{>/etc/hostname}}')) > 0 AS whitelist_grants_unprivileged_nonempty;
SELECT mustach('{}', E'{{>/etc/passwd}}');

--
-- pg_mustach.whitelist is registered PGC_SUSET, so only a superuser can set
-- it -- confirm that mustach_test_none, a genuine non-superuser, can't
-- loosen its own scope with a plain SET.
--
SELECT set_config('pg_mustach.whitelist', 'file:///etc/passwd', false);

\c - :pg_mustach_test_orig_user
ALTER ROLE mustach_test_none RESET pg_mustach.whitelist;

--
-- pg_mustach.whitelist (see the pg_whitelist submodule) adds an optional
-- restriction on top of superuser status: even mustach_test_full, which is
-- a superuser, can be scoped down to specific file:// prefixes. ALTER
-- ROLE ... SET only takes effect for a *new* connection as that role
-- (role-level GUC defaults are applied once, in InitPostgres, at backend
-- startup) -- unlike the plain role-membership check above, SET ROLE within
-- this already-open session will NOT pick it up, so exercising it means
-- reconnecting via \c.
--

--
-- Without any pg_mustach.whitelist, being a superuser alone is enough.
--
SET ROLE mustach_test_full;
SELECT octet_length(mustach('{}', E'{{>/etc/hostname}}')) > 0 AS role_only_nonempty;
RESET ROLE;

--
-- An exact file:// entry permits only that file.
--
ALTER ROLE mustach_test_full SET pg_mustach.whitelist = 'file:///etc/hostname';
\c - mustach_test_full

SELECT octet_length(mustach('{}', E'{{>/etc/hostname}}')) > 0 AS whitelist_exact_match_nonempty;
SELECT mustach('{}', E'{{>/etc/passwd}}');

--
-- A file:// entry with a trailing slash permits anything under that
-- directory, but the resolved path is realpath()-canonicalized before
-- comparison, so ".." can't be used to climb back out of it even though the
-- raw string would otherwise start with the whitelisted prefix.
--
\c - :pg_mustach_test_orig_user
ALTER ROLE mustach_test_full SET pg_mustach.whitelist = 'file:///etc/ssl/';
\c - mustach_test_full

SELECT octet_length(mustach('{}', E'{{>/etc/ssl/openssl.cnf}}')) > 100 AS whitelist_directory_match_nonempty;
SELECT mustach('{}', E'{{>/etc/hostname}}');
SELECT mustach('{}', E'{{>/etc/ssl/../passwd}}');

\c - :pg_mustach_test_orig_user
ALTER ROLE mustach_test_full RESET pg_mustach.whitelist;

DROP ROLE mustach_test_none;
DROP ROLE mustach_test_full;
DROP EXTENSION pg_mustach;
