\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'array-each', mustach('{"names": [{"name": "Moe"}, {"name": "Larry"}, {"name": "Curly"}, {"name": "Shemp"}]}', '{{#names}}{{name}}{{/names}}');
SELECT 2, 'complex', mustach('{"header": "Colors", "hasItems": true, "items": [{"name": "red", "current": true, "url": "#Red"}, {"name": "green", "current": false, "url": "#Green"}, {"name": "blue", "current": false, "url": "#Blue"}]}', '<h1>{{header}}</h1>
{{#hasItems}}
  <ul>
    {{#items}}
      {{#current}}
        <li><strong>{{name}}</strong></li>
      {{/current}}
      {{^current}}
        <li><a href="{{url}}">{{name}}</a></li>
      {{/current}}
    {{/items}}
  </ul>
{{/hasItems}}');
SELECT 3, 'data', mustach('{"names": [{"name": "Moe"}, {"name": "Larry"}, {"name": "Curly"}, {"name": "Shemp"}]}', '{{#names}}{{@index}}{{name}}{{/names}}');
SELECT 4, 'depth-1', mustach('{"names": [{"name": "Moe"}, {"name": "Larry"}, {"name": "Curly"}, {"name": "Shemp"}], "foo": "bar"}', '{{#names}}{{foo}}{{/names}}');
SELECT 5, 'depth-2', mustach('{"names": [{"bat": "foo", "name": ["Moe"]}, {"bat": "foo", "name": ["Larry"]}, {"bat": "foo", "name": ["Curly"]}, {"bat": "foo", "name": ["Shemp"]}], "foo": "bar"}', '{{#names}}{{#name}}{{bat}}{{foo}}{{/name}}{{/names}}');
SELECT 6, 'object', mustach('{"person": {"name": "Larry", "age": 45}}', '{{#person}}{{name}}{{age}}{{/person}}');
SELECT 7, 'object-mustach', mustach('{"person": {"name": "Larry", "age": 45}}', '{{#person}}{{name}}{{age}}{{/person}}');
SELECT 8, 'partial', mustach('{"peeps": [{"name": "Moe", "count": 15}, {"name": "Larry", "count": 5}, {"name": "Curly", "count": 1}]}', '{{#peeps}}{{>variables}}{{/peeps}}');
SELECT 9, 'partial-recursion', mustach('{"name": "1", "kids": [{"name": "1.1", "kids": [{"name": "1.1.1", "kids": []}]}]}', '{{name}}{{#kids}}{{>recursion}}{{/kids}}');
SELECT 10, 'paths', mustach('{"person": {"name": {"bar": {"baz": "Larry"}}, "age": 45}}', '{{person.name.bar.baz}}{{person.age}}{{person.foo}}{{animal.age}}');
SELECT 11, 'string', mustach('{}', 'Hello world');
SELECT 12, 'variables', mustach('{"name": "Mick", "count": 30}', 'Hello {{name}}! You have {{count}} new messages.');
ROLLBACK;
