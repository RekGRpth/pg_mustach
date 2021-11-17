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
ROLLBACK;
