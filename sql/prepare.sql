\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
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
ROLLBACK;
