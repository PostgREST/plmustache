# plmustache

A PostgreSQL extension that provides a language handler for [Mustache](https://mustache.github.io/) templates using https://gitlab.com/jobol/mustach.

## Roadmap

- [x] variable interpolation
- [x] sections
  - [x] bools
  - [ ] arrays
- [ ] partials
- [ ] inheritance
- [ ] lambdas

## Features

### Variables

Variables are handled as per the [mustache spec](https://mustache.github.io/mustache.5.html), a `{{key}}` variable will be interpolated.

```sql
create or replace function win_money(you text, qt money, at timestamptz) returns text as $$
Hello {{you}}!
You just won {{qt}} at {{at}}.
$$ language plmustache;

select win_money('Sir Meowalot', '12000', now());
                         win_money
-----------------------------------------------------------
 Hello Sir Meowalot!                                            +
 You just won $12,000.00 at 2023-12-04 07:44:26.915735-05.
(1 row)
```

#### Escaped and Unescaped

A double mustache `{{key}}` will be escaped and a triple mustache `{{{key}}}` will not be escaped.

```sql
create or replace function escape_me(tag text) returns text as $$
{{tag}}
$$ language plmustache;

select escape_me('<script>evil()</script>');
              escape_me
-------------------------------------
 &lt;script&gt;evil()&lt;/script&gt;
(1 row)

create or replace function do_not_escape_me(tag text) returns text as $$
{{{tag}}}
$$ language plmustache;

select do_not_escape_me('<script>evil()</script>');
    do_not_escape_me
-------------------------
 <script>evil()</script>
(1 row)
```

### Sections

Boolean sections:

```sql
create or replace function show_cat(cat text, show bool default true) returns text as $$
{{#show}}
A cat appears, it's {{cat}}.
{{/show}}
{{^show}}
A mysterious cat is hiding.
{{/show}}
$$ language plmustache;

select show_cat('Mr. Sleepy');
            show_cat
---------------------------------
 A cat appears, it's Mr. Sleepy.+

(1 row)

select show_cat('Mr. Sleepy', false);
          show_cat
-----------------------------
 A mysterious cat is hiding.+

(1 row)
```

Array iterators:

```sql
create or replace function hello_cats(cats text[]) returns text as $$
Say hello to: {{#cats}}{{.}}, {{/cats}}
$$ language plmustache;


postgres=# select hello_cats('{Sir Meowalot, Mr. Sleepy, Paquito}');
                    hello_cats
---------------------------------------------------
 Say hello to: Sir Meowalot, Mr. Sleepy, Paquito,
(1 row)
```

## Installation

Clone the repo and submodules:

```
git clone --recurse-submodules https://github.com/PostgREST/plmustache
```

Build mustach:

```
cd mustach
make && sudo make install
sudo ldconfig
```

Build plmustache:

```
cd ..

make && sudo make install
```

Then on SQL you can do:

```sql
CREATE EXTENSION plmustache;
```

plmustache is tested on Postgres 12, 13, 14, 15, 16.

## Development

For testing on your local database:

```
make installcheck
```

For an isolated and reproducible enviroment you can use [Nix](https://nixos.org/download.html).

```
$ nix-shell

$ with-pg-15 psql

$ with-pg-15 make installcheck
```
