\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
CREATE EXTENSION pg_mustach;

--
-- pg_mustach.transaction defaults to true, mirroring pg_curl.transaction:
-- a prepared template lives only as long as the transaction it was
-- prepared in. Each of these SELECTs autocommits as its own transaction,
-- so the template from the first one is gone by the second.
--
SELECT mustach_prepare('{{a}}', 'txn_autocommit');
\set ON_ERROR_STOP false
SELECT 1, 'a template prepared in one autocommitted statement is gone by the next', mustach_render('{"a":"b"}', tplname := 'txn_autocommit');
\set ON_ERROR_STOP true

--
-- Same default (true), but within one explicit transaction the template
-- survives until COMMIT/ROLLBACK, same as before this GUC existed.
--
BEGIN;
SELECT mustach_prepare('{{a}}', 'txn_explicit');
SELECT 2, 'within one explicit transaction the template is still there', mustach_render('{"a":"b"}', tplname := 'txn_explicit');
COMMIT;
\set ON_ERROR_STOP false
SELECT 3, 'gone once that transaction commits', mustach_render('{"a":"b"}', tplname := 'txn_explicit');
\set ON_ERROR_STOP true

BEGIN;
SELECT mustach_prepare('{{a}}', 'txn_rollback');
ROLLBACK;
\set ON_ERROR_STOP false
SELECT 4, 'also gone if that transaction rolls back instead', mustach_render('{"a":"b"}', tplname := 'txn_rollback');
\set ON_ERROR_STOP true

--
-- The unnamed default slot is scoped the same way as named templates.
--
SELECT mustach_prepare('{{a}}');
\set ON_ERROR_STOP false
SELECT 5, 'the unnamed default slot is transaction-scoped too', mustach_render('{"a":"b"}');
\set ON_ERROR_STOP true

--
-- pg_mustach.transaction = false restores session-lifetime templates,
-- surviving across autocommitted statements and explicit transactions
-- alike, same as before this GUC was added.
--
SET pg_mustach.transaction = false;
SELECT mustach_prepare('{{a}}', 'txn_session');
SELECT 6, 'with pg_mustach.transaction off, the template survives across statements', mustach_render('{"a":"b"}', tplname := 'txn_session');
BEGIN;
SELECT 7, 'and across an explicit transaction boundary', mustach_render('{"a":"c"}', tplname := 'txn_session');
COMMIT;
SELECT 8, 'still there after that commit', mustach_render('{"a":"d"}', tplname := 'txn_session');
SELECT mustach_forget('txn_session');
\set ON_ERROR_STOP false
SELECT 9, 'explicit forget still removes it immediately, no waiting for a transaction boundary', mustach_render('{"a":"e"}', tplname := 'txn_session');
\set ON_ERROR_STOP true
RESET pg_mustach.transaction;

DROP EXTENSION pg_mustach;
