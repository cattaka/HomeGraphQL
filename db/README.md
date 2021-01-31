## How to add the new user_account

```sql
insert into user_accounts(
  userrole,
  is_admin,
  username,
  password_salt
) values (
    'home_graph_ql_read',
    false,
    'new_username',
    gen_salt('md5')
);
update user_accounts SET password_hash = crypt('new_password', password_salt) where username = 'new_username'
```
