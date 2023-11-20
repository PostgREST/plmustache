create extension if not exists plmustache;
\echo

create or replace function bool_section(x1 text, x2 bool) returns text as $$
Section: {{#x2}}Show {{x1}}{{/x2}}
$$ language plmustache;
\echo

select bool_section('Something', true);

select bool_section('Something', false);

create or replace function bool_section_inverted(x1 text, x2 bool) returns text as $$
Section: Show {{#x2}}{{x1}}{{/x2}}{{^x2}}Nothing{{/x2}}
$$ language plmustache;
\echo

select bool_section_inverted('Something', true);

select bool_section_inverted('Something', false);

create or replace function foo_test(foo text) returns text as $$
{{#foo}}foo is {{foo}}{{/foo}}
$$ language plmustache;
\echo

select foo_test('bar');

create or replace function foo_test(foo bool) returns text as $$
foo is {{#foo}}{{foo}}{{/foo}}{{^foo}}{{foo}}{{/foo}}
$$ language plmustache;
\echo

select foo_test(true);

select foo_test(false);

create or replace function foo_null_test(foo bool) returns text as $$
foo is {{#foo}}full{{/foo}}{{^foo}}null{{/foo}}
$$ language plmustache;
\echo

select foo_null_test(null);

select foo_null_test(true);

create or replace function foo_array(arr text[]) returns text as $$
foo is {{#arr}}{{.}}, {{/arr}}
$$ language plmustache;
\echo

select foo_array(ARRAY['one', 'two', 'three']);

create or replace function foo_array(arr int[]) returns text as $$
foo is {{#arr}}{{.}}, {{/arr}}
$$ language plmustache;
\echo

select foo_array(ARRAY[1, 2, 3]::int[]);

create or replace function mixed_var_array(var int, arr int[]) returns text as $$
bar is {{var}}, foo is {{#arr}}{{.}}, {{/arr}}
$$ language plmustache;
\echo

select mixed_var_array(4, ARRAY[1, 2, 3]::int[]);

create or replace function mixed_array_var(arr int[], var int) returns text as $$
foo is {{#arr}}{{.}}, {{/arr}} bar is {{var}}
$$ language plmustache;
\echo

select mixed_array_var(ARRAY[1, 2, 3]::int[], 4);
