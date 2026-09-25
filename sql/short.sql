CREATE EXTENSION pg_mustach;
SELECT 1, 'content json', mustach('{"a":"b"}', '{{a}}');
SELECT 2, 'content json', mustach('{"people":[{"firstName":"Yehuda","lastName":"Katz"},{"firstName":"Carl","lastName":"Lerche"},{"firstName":"Alan","lastName":"Johnson"}]}', '<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>');
SELECT 3, 'an empty template renders empty, not whatever follows it in memory', '[' || mustach('{"a":"b"}', '') || ']';
\! rm -f /tmp/pg_mustach_test_empty.txt
SELECT 4, 'an empty template writes an empty file', mustach('{"a":"b"}', '', '/tmp/pg_mustach_test_empty.txt');
\! printf '5|empty template file size|%s\n' "$(wc -c < /tmp/pg_mustach_test_empty.txt)"; rm -f /tmp/pg_mustach_test_empty.txt
SELECT 6, 'an empty string value renders empty, not whatever follows it in the jsonb', '[' || mustach('{"a":"","b":"XYZ"}', '{{a}}') || ']';
SELECT 7, 'same, unescaped', '[' || mustach('{"a":"","b":"XYZ"}', '{{{a}}}') || ']';
SELECT 8, 'same, as an array element', '[' || mustach('{"a":["","XYZ"]}', '{{#a}}<{{.}}>{{/a}}') || ']';
SELECT 9, 'same, as a partial from the data', '[' || mustach('{"a":"","b":"XYZ"}', '{{>a}}') || ']';
SELECT 10, 'an empty key renders empty under object iteration', '[' || mustach('{"":"v","b":"XYZ"}', '{{#*}}<{{*}}={{.}}>{{/*}}') || ']';
SELECT 11, 'an empty string as the last thing in the jsonb renders empty', '[' || mustach('{"a":""}', '{{a}}') || ']';
DROP EXTENSION pg_mustach;
