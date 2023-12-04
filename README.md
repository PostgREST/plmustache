# plmustache

Logic-less templates for Postgres. Tested on Postgres 12, 13, 14, 15, 16.

## Roadmap

- [x] interpolation
- [x] sections
  - [x] bools
  - [ ] arrays
- [ ] partials
- [ ] inheritance
- [ ] lambdas

## Features

### Variables

```sql
create or replace function win_money(you text, qt money, at timestamptz) returns text as $$
Hello {{you}}!
You just won {{qt}} at {{at}}.
$$ language plmustache;

select win_money('Slonik', '12000', now());
                         win_money
-----------------------------------------------------------
 Hello Slonik!                                            +
 You just won $12,000.00 at 2023-12-04 07:44:26.915735-05.
(1 row)
```

### Sections

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

## Installation

Install https://gitlab.com/jobol/mustach, then on this repo:

```
$ make && make install
```

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
