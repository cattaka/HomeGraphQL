CREATE EXTENSION pgcrypto;

-- https://www.graphile.org/postgraphile/security/
create type jwt_token as (
  role text,
  exp integer,
  person_id integer,
  is_admin boolean,
  username varchar
);

create table user_accounts (
  id serial primary key,
  is_admin boolean not null,
  username text not null unique,
  userrole text not null unique,
  password_hash varchar,
  password_salt varchar
);

create function authenticate(username text, password text) returns jwt_token
    strict
    security definer
    language plpgsql
as
$$
declare
  account user_accounts;
begin
  select a.* into account
    from user_accounts as a
    where a.username = authenticate.username;

  if account.password_hash = crypt(password, account.password_salt) then
    return (
      account.userrole,
      extract(epoch from now() + interval '370 days'),
      account.id,
      account.is_admin,
      account.username
    )::jwt_token;
  else
    return null;
  end if;
end;
$$;
alter function authenticate(text, text) owner to home_graph_ql;

-- My data table
create table temp_humi_values (
  id serial primary key,
  sensor_name text not null,
  temperature numeric not null,
  humidity numeric not null,
  pressure numeric not null,
  co2 numeric not null,
  updated_at timestamptz not null default now()
);
create index index_temp_humi_values ON temp_humi_values (sensor_name, updated_at);

-- Rols for access controll
create role home_graph_ql_authenticater;
create role home_graph_ql_read;
create role home_graph_ql_write;
grant execute
  on function public.authenticate
  to home_graph_ql_authenticater;
grant select
  on public.temp_humi_values
  to home_graph_ql_read;
grant select, insert, update, delete
  on public.temp_humi_values
  to home_graph_ql_write;
grant select, update
  on sequence public.temp_humi_values_id_seq
  to home_graph_ql_write;
grant home_graph_ql_authenticater to home_graph_ql;
grant home_graph_ql_read to home_graph_ql;
grant home_graph_ql_write to home_graph_ql;
