CREATE EXTENSION pg_mustach;
SELECT 1, 'section nesting one level below the runtime depth limit renders', mustach('{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":true}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}', '{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}X{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}');
SELECT 2, 'section nesting at the runtime depth limit errors', mustach('{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":true}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}', '{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}{{#a}}X{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}{{/a}}');
--
-- Templates are encoded in blocks of 1024 words; sections ending on, or
-- skipped across, block boundaries used to jump to garbage addresses in
-- libmustach -- MUSTACH_ERROR_CLOSING from 722 skipped sections in a row
-- on, or from a skipped section whose body spans 3+ blocks. Needs a
-- libmustach with the fix.
--
SELECT 3, 'thousands of skipped sections in a row render empty', '[' || mustach('{}', repeat('{{#a}}x{{/a}}', 5000)) || ']';
SELECT 4, 'a skipped section whose body spans several blocks is jumped over', mustach('{"b":"B"}', '[{{#a}}' || repeat('x{{b}}', 3000) || '{{/a}}]{{b}}');
SELECT 5, 'thousands of entered sections in a row render', length(mustach('{"b":1}', repeat('{{#b}}y{{/b}}', 5000)));
DROP EXTENSION pg_mustach;
