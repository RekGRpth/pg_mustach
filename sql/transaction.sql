\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
--
-- Before PG 9.5 (no MemoryContextRegisterResetCallback) pg_mustach.transaction
-- has no effect and every prepared template is session-lifetime regardless --
-- see expected/transaction_1.out for the matching alternate output.
--
CREATE EXTENSION pg_mustach;

--
-- pg_mustach.transaction defaults to true, mirroring pg_curl.transaction:
-- a prepared template lives only as long as the transaction it was
-- prepared in. Each of these SELECTs autocommits as its own transaction,
-- so the template from the first one is gone by the second.
--
SELECT mustach_template('{{a}}', 'txn_autocommit');
\set ON_ERROR_STOP false
SELECT 1, 'a template prepared in one autocommitted statement is gone by the next', mustach_json('{"a":"b"}', tplname := 'txn_autocommit');
\set ON_ERROR_STOP true

--
-- Same default (true), but within one explicit transaction the template
-- survives until COMMIT/ROLLBACK, same as before this GUC existed.
--
BEGIN;
SELECT mustach_template('{{a}}', 'txn_explicit');
SELECT 2, 'within one explicit transaction the template is still there', mustach_json('{"a":"b"}', tplname := 'txn_explicit');
COMMIT;
\set ON_ERROR_STOP false
SELECT 3, 'gone once that transaction commits', mustach_json('{"a":"b"}', tplname := 'txn_explicit');
\set ON_ERROR_STOP true

BEGIN;
SELECT mustach_template('{{a}}', 'txn_rollback');
ROLLBACK;
\set ON_ERROR_STOP false
SELECT 4, 'also gone if that transaction rolls back instead', mustach_json('{"a":"b"}', tplname := 'txn_rollback');
\set ON_ERROR_STOP true

--
-- The unnamed default slot is scoped the same way as named templates.
--
SELECT mustach_template('{{a}}');
\set ON_ERROR_STOP false
SELECT 5, 'the unnamed default slot is transaction-scoped too', mustach_json('{"a":"b"}');
\set ON_ERROR_STOP true

--
-- pg_mustach.transaction = false restores session-lifetime templates,
-- surviving across autocommitted statements and explicit transactions
-- alike, same as before this GUC was added. Before PG 9.5 the GUC isn't
-- registered at all (see _PG_init) so this SET is just a harmless
-- placeholder assignment (PostgreSQL accepts SET on any dotted name since
-- 9.2's removal of custom_variable_classes) -- session-lifetime is already
-- the only behavior there regardless.
--
SET pg_mustach.transaction = false;
SELECT mustach_template('{{a}}', 'txn_session');
SELECT 6, 'with pg_mustach.transaction off, the template survives across statements', mustach_json('{"a":"b"}', tplname := 'txn_session');
BEGIN;
SELECT 7, 'and across an explicit transaction boundary', mustach_json('{"a":"c"}', tplname := 'txn_session');
COMMIT;
SELECT 8, 'still there after that commit', mustach_json('{"a":"d"}', tplname := 'txn_session');
SELECT mustach_free('txn_session');
\set ON_ERROR_STOP false
SELECT 9, 'explicit forget still removes it immediately, no waiting for a transaction boundary', mustach_json('{"a":"e"}', tplname := 'txn_session');
\set ON_ERROR_STOP true
RESET pg_mustach.transaction;

--
-- The scope is decided per template, by pg_mustach.transaction as it is
-- when mustach_template() prepares it: changing the setting mid-session
-- applies to templates prepared from then on, in either direction, and
-- leaves those already prepared alone.
--
SET pg_mustach.transaction = false;
SELECT mustach_template('{{a}}', 'mix_session');
SELECT mustach_template('{{a}}');
RESET pg_mustach.transaction;
SELECT mustach_template('{{a}}', 'mix_transaction');
\set ON_ERROR_STOP false
SELECT 10, 'turned on after being off, templates prepared from then on are transaction-scoped', mustach_json('{"a":"b"}', tplname := 'mix_transaction');
\set ON_ERROR_STOP true
SELECT 11, 'while the one prepared with it off stays session-scoped', mustach_json('{"a":"b"}', tplname := 'mix_session');
SELECT 12, 'and so does the unnamed default slot prepared with it off', mustach_json('{"a":"b"}');

BEGIN;
SELECT mustach_template('{{a}}', 'mix_before');
SET pg_mustach.transaction = false;
SELECT mustach_template('{{a}}', 'mix_after');
COMMIT;
\set ON_ERROR_STOP false
SELECT 13, 'turned off mid-transaction, the template prepared before that is still gone at commit', mustach_json('{"a":"b"}', tplname := 'mix_before');
\set ON_ERROR_STOP true
SELECT 14, 'while the one prepared after it survives', mustach_json('{"a":"b"}', tplname := 'mix_after');

SELECT mustach_template('{{a}}', 'mix_session');
RESET pg_mustach.transaction;
SELECT mustach_template('{{a}}', 'mix_session');
\set ON_ERROR_STOP false
SELECT 15, 're-preparing a tplname takes the scope in effect at that point', mustach_json('{"a":"b"}', tplname := 'mix_session');
\set ON_ERROR_STOP true
SELECT 16, 'session-scoped templates are still there to forget', mustach_free('mix_after') AND mustach_free();

DROP EXTENSION pg_mustach;
