create extension if not exists plmustache;
\echo

create or replace function hello() returns text as
$$Hello, you$$
language plmustache;
\echo

select hello();

create or replace function hello(x1 text) returns text as
$$Hello, {{x1}}$$
language plmustache;
\echo

select hello('world');

create or replace function hello(x1 text, x2 text) returns text as
$$Hello, {{x1}} and {{x2}}$$
language plmustache;
\echo

select hello('Jane', 'John');

create or replace function hello(x1 int, x2 float, x3 json) returns text as
$$Hello, {{x1}}, {{x2}} and {{x3}}$$
language plmustache;
\echo

select hello(1, 3.40, '{"abc": "xyz"}');

create or replace function hello_not_found_key(x1 int) returns text as
$$Hello, {{not_found}}$$
language plmustache;
\echo

select hello_not_found_key(1);

-- surrounding newlines are stripped, but not inside newlines
create or replace function haiku(x1 text, x2 text, x3 text) returns text as $$
{{x1}}
{{x2}}
{{x3}}
$$
language plmustache;
\echo

select haiku('The earth shakes', 'just enough', 'to remind us');

DO language plmustache $$ Hello $$;
\echo

create or replace function hello(p point) returns text as $$
Hello, {{p}}
$$ language plmustache;
\echo

select hello(point(3,4));

create or replace function hello(x int[]) returns text as $$
Hello, {{x}}
$$ language plmustache;
\echo

select hello(ARRAY[1,2,3]);

create function hello_no_ret_text(x text) returns int as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_no_ret_text(x text) returns float as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_no_ret_text(x text) returns trigger as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_no_ret_text(x text) returns event_trigger as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_out(out x line) as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_inout(inout x line) as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_nonnamed_param(text) returns text as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_nonnamed_param(x text, text) returns text as $$
Hello, {{x}}
$$ language plmustache;
\echo

create function hello_nonnamed_param(text, y text) returns text as $$
Hello, {{x}}
$$ language plmustache;
\echo

create domain "text/plain" as text;
create or replace function hi() returns "text/plain" as
$$Hi, you$$
language plmustache;
\echo

-- validation happens even at runtime in case check_function_bodies is turned off
set check_function_bodies to off;
\echo

create function hello_invalid(text) returns int as $$
Hello, {{x}}
$$ language plmustache;
\echo

select hello_invalid('foo');
\echo

set check_function_bodies to on;
