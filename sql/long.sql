\unset ECHO
\set QUIET 1
\pset format unaligned
\pset tuples_only true
\pset pager off
\set ON_ERROR_ROLLBACK 1
\set ON_ERROR_STOP true
BEGIN;
CREATE EXTENSION pg_mustach;
SELECT 1, 'mustach', mustach('{
  "name": "Chris",
  "value": 10000,
  "taxed_value": 6000,
  "in_ca": true,
  "person": false,
  "repo": [
    { "name": "resque", "who": [ { "commiter": "joe" }, { "reviewer": "avrel" }, { "commiter": "william" } ] },
    { "name": "hub", "who": [ { "commiter": "jack" }, { "reviewer": "avrel" }, { "commiter": "greg" } ]  },
    { "name": "rip", "who": [ { "reviewer": "joe" }, { "reviewer": "jack" }, { "commiter": "greg" } ]  }
  ],
  "person?": { "name": "Jon" },
  "specia": "----{{extra}}----\n",
  "extra": 3.14159,
  "#sharp": "#",
  "!bang": "!",
  "/slash": "/",
  "^circ": "^",
  "=equal": "=",
  ":colon": ":",
  ">greater": ">",
  "~tilde": "~"
}', 'Hello {{name}}
You have just won {{value}} dollars!
{{#in_ca}}
Well, {{taxed_value}} dollars, after taxes.
{{/in_ca}}
Shown.
{{#person}}
  Never shown!
{{/person}}
{{^person}}
  No person
{{/person}}

{{#repo}}
  <b>{{name}}</b> reviewers:{{#who}} {{reviewer}}{{/who}} commiters:{{#who}} {{commiter}}{{/who}}
{{/repo}}

{{#person?}}
  Hi {{name}}!
{{/person?}}

{{=%(% %)%=}}
 =====================================
%(%! gros commentaire %)%
%(%#repo%)%
  <b>%(%name%)%</b> reviewers:%(%#who%)% %(%reviewer%)%%(%/who%)% commiters:%(%#who%)% %(%commiter%)%%(%/who%)%
%(%/repo%)%
 =====================================
%(%={{ }}=%)%
ggggggggg
{{> specia}}
jjjjjjjjj
end

{{:#sharp}}
{{:!bang}}
{{:~tilde}}
{{:/~0tilde}}
{{:/~1slash}} see json pointers IETF RFC 6901
{{:^circ}}
{{:\=equal}}
{{::colon}}
{{:>greater}}');
SELECT 2, 'mustach', mustach('{
  "header": "Colors",
  "items": [
      {"name": "red", "first": true, "url": "#Red"},
      {"name": "green", "link": true, "url": "#Green"},
      {"name": "blue", "link": true, "url": "#Blue"}
  ],
  "empty": false
}', '<h1>{{header}}</h1>
{{#bug}}
{{/bug}}

{{#items}}
  {{#first}}
    <li><strong>{{name}}</strong></li>
  {{/first}}
  {{#link}}
    <li><a href="{{url}}">{{name}}</a></li>
  {{/link}}
{{/items}}

{{#empty}}
  <p>The list is empty.</p>
{{/empty}}');
SELECT 3, 'mustach', mustach('{
  "name": "Chris",
  "company": "<b>GitHub & Co</b>",
  "names": ["Chris", "Kross"],
  "skills": ["JavaScript", "PHP", "Java"],
  "age": 18
}', '* {{name}}
* {{age}}
* {{company}}
* {{&company}}
* {{{company}}}
{{=<% %>=}}
* <%company%>
* <%&company%>
* <%{company}%>

<%={{ }}=%>
* <ul>{{#names}}<li>{{.}}</li>{{/names}}</ul>
* skills: <ul>{{#skills}}<li>{{.}}</li>{{/skills}}</ul>
{{#age}}* age: {{.}}{{/age}}');
ROLLBACK;
