# pg_mustach

PostgreSQL implementation of [mustache](https://mustache.github.io/) templating, rendering
against `jsonb` and supporting most of the common Handlebars-style extensions (else blocks,
comparisons, JSON Pointer paths, object iteration, colon-partials, ...).

## Install

Standard PGXS build (requires [libmustach](https://gitlab.com/jobol/mustach) >= 2 and its headers
installed, plus the `pg_whitelist` git submodule checked out):

```sh
git submodule update --init
make
make install
```

Then in `psql`:

```sql
CREATE EXTENSION pg_mustach;
```

## Rendering

```sql
SELECT mustach('{"a":"b"}', '{{a}}');
--  b

SELECT mustach('{"people":[{"firstName":"Yehuda","lastName":"Katz"}]}',
               '<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>');
--  <ul><li>Yehuda Katz</li></ul>
```

- `mustach(json jsonb, template text) RETURNS text` — renders `template` against `json` and
  returns the result.
- `mustach(json jsonb, template text, file text) RETURNS bool` — same, but writes the result to
  `file` on the server instead of returning it. Restricted to superusers.

### Rendering the same template many times

Parsing happens on every `mustach()` call. When the same template is rendered against many
different `jsonb` values (e.g. once per row), parse it once instead, giving it a name (analogous
to `conname` in [pg_curl](https://github.com/RekGRpth/pg_curl)):

```sql
SELECT mustach_prepare('<ul>{{#people}}<li>{{firstName}} {{lastName}}</li>{{/people}}</ul>', 'people');

SELECT mustach_render(data, tplname := 'people') FROM my_table;

SELECT mustach_forget('people');
```

`tplname` is optional everywhere it appears (`DEFAULT NULL`); omitting it addresses a single
unnamed default slot, same as omitting `conname` addresses pg_curl's default connection:

```sql
SELECT mustach_prepare('{{a}}');
SELECT mustach_render('{"a":"b"}');  -- b
```

- `mustach_prepare(template text, tplname name DEFAULT NULL) RETURNS void` — parses `template`
  and stores it under `tplname`. Preparing again under the same `tplname` replaces whatever was
  there.
- `mustach_render(json jsonb, tplname name DEFAULT NULL) RETURNS text` — renders the template
  prepared under `tplname` against `json`.
- `mustach_render(json jsonb, file text, tplname name DEFAULT NULL) RETURNS bool` — same, but
  writes the result to `file` on the server instead of returning it, same restrictions as the
  3-argument `mustach()` above (superuser only).

> [!WARNING]
> Always pass `tplname` by name (`tplname := 'people'`), never as a bare second positional
> argument. `mustach_render(json, 'people')` looks like a call to the two-argument form above,
> but PostgreSQL's overload resolution prefers `text` over `name` for an unknown-type string
> literal, so a bare positional second argument actually resolves to the three-argument
> `file`-writing overload — `'people'` becomes the *file path*, not the template name, and it
> silently writes a file on the server instead of erroring. Named-argument syntax sidesteps this
> because it requires an actual parameter called `tplname`, which the `file`-writing overload
> only has once `file` itself is otherwise supplied.
- `mustach_forget(tplname name DEFAULT NULL) RETURNS bool` — releases a prepared template,
  returning whether `tplname` was still known. Prepared templates are backend-local (not visible
  from other sessions) and live until forgotten or the session ends, so long-lived sessions that
  keep preparing under new names without forgetting will accumulate memory.

## Extensions and flags

Rendering behavior is controlled by a bitmask of flags, read from the `pg_mustach.flags` GUC
(default: all extensions enabled) or overridden per-session/-transaction with
`mustach_set_flags(flags int, is_local bool DEFAULT false)`. Each flag has a matching accessor:

| Function                             | Enables                                                |
|---------------------------------------|---------------------------------------------------------|
| `mustach_with_allextensions()`        | all of the below                                         |
| `mustach_with_noextensions()`         | none of the below (strict mustache)                      |
| `mustach_with_colon()`                | `{{:name}}` partial-with-indentation syntax               |
| `mustach_with_compare()`              | `{{#x<y}}`/`{{#x>y}}`/`{{#x<=y}}`/`{{#x>=y}}` section comparisons |
| `mustach_with_equal()`                | `{{#x=y}}` section equality comparisons                   |
| `mustach_with_emptytag()`             | allow `{{}}` instead of erroring                          |
| `mustach_with_errorundefined()`       | error instead of rendering empty on an undefined tag       |
| `mustach_with_escfirstcmp()`          | allow escaping a leading comparison operator in a key      |
| `mustach_with_incpartial()`           | include the partial's own data scope while resolving it    |
| `mustach_with_jsonpointer()`          | `{{/a/b}}` JSON Pointer paths instead of dotted paths       |
| `mustach_with_objectiter()`           | `{{#*}}` iterate over an object's key/value pairs           |
| `mustach_with_partialdatafirst()`     | look up `{{>name}}` in the json data before the filesystem  |
| `mustach_with_singledot()`            | `{{.}}` refers to the current selection                    |

```sql
SELECT mustach_set_flags(mustach_with_noextensions());
SELECT mustach('{"s":"abc"}', '{{#s=abc}}yes{{/s=abc}}');  -- errors: = is an extension
```

## Partials and `pg_mustach.whitelist`

`{{>name}}` first looks for `name` in the json data; if not found there, it falls back to
reading `name` (or `name.mustache`) as a file path on the server. That local-file fallback is
gated through the [`pg_whitelist`](pg_whitelist/) submodule via the `pg_mustach.whitelist` GUC
(superuser-settable only):

- Superusers may read any local file unless `pg_mustach.whitelist` explicitly excludes it.
- Non-superusers may read a local file only if `pg_mustach.whitelist` explicitly includes it
  (as a `file://` prefix; a trailing slash allows anything under that directory).

```sql
ALTER ROLE some_role SET pg_mustach.whitelist = 'file:///etc/ssl/';
```

`mustach(json, template, file)` (the 3-argument, file-writing form) is unconditionally
restricted to superusers, independent of the whitelist.

## Testing

```sh
make installcheck
```
