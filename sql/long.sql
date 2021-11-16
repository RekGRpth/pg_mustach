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
SELECT 4, 'mustach', mustach('{
  "person": { "name": "Jon", "age": 25 },
  "person.name": "Fred",
  "person.name=Fred": "The other Fred.",
  "persons": [
	{ "name": "Jon", "age": 25, "lang": "en" },
	{ "name": "Henry", "age": 27, "lang": "en" },
	{ "name": "Amed", "age": 24, "lang": "fr" } ],
  "fellows": {
	"Jon": { "age": 25, "lang": "en" },
	"Henry": { "age": 27, "lang": "en" },
	"Amed": { "age": 24, "lang": "fr" } }
}', 'This are extensions!!

{{person.name}}
{{person.age}}

{{person\.name}}
{{person\.name\=Fred}}

{{#person.name=Jon}}
Hello Jon
{{/person.name=Jon}}

{{^person.name=Jon}}
No Jon? Hey Jon...
{{/person.name=Jon}}

{{^person.name=Harry}}
No Harry? Hey Calahan...
{{/person.name=Harry}}

{{#person\.name=Fred}}
Hello Fred
{{/person\.name=Fred}}

{{^person\.name=Fred}}
No Fred? Hey Fred...
{{/person\.name=Fred}}

{{#person\.name\=Fred=The other Fred.}}
Hello Fred#2
{{/person\.name\=Fred=The other Fred.}}

{{^person\.name\=Fred=The other Fred.}}
No Fred#2? Hey Fred#2...
{{/person\.name\=Fred=The other Fred.}}

{{#persons}}
{{#lang=!fr}}Hello {{name}}, {{age}} years{{/lang=!fr}}
{{#lang=fr}}Salut {{name}}, {{age}} ans{{/lang=fr}}
{{/persons}}

{{#persons}}
{{name}}: {{age=24}}/{{age}}/{{age=!27}}
{{/persons}}

{{#fellows.*}}
{{*}}: {{age=24}}/{{age}}/{{age=!27}}
{{/fellows.*}}

{{#*}}
 (1) {{*}}: {{.}}
   {{#*}}
     (2) {{*}}: {{.}}
     {{#*}}
       (3) {{*}}: {{.}}
     {{/*}}
   {{/*}}
{{/*}}');
SELECT 5, 'mustach', mustach('{
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
  "specia": "----{{extra}}----",
  "extra": 3.14159,
  "#sharp": "#",
  "!bang": "!",
  "/slash": "/",
  "^circ": "^",
  "=equal": "=",
  ":colon": ":",
  ">greater": ">",
  "~tilde": "~"
}', ' =====================================
from json
{{> specia}}
 =====================================
not found
{{> notfound}}
 =====================================
without extension first
{{> must2 }}
 =====================================
last with extension
{{> must3 }}
 =====================================
Ensure must3 didn''t change specials

{{#person?}}
  Hi {{name}}!
{{/person?}}

%(%#person?%)%
  Hi %(%name%)%!
%(%/person?%)%');
ROLLBACK;
