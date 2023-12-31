```sql

define table user schemaful;

define field email on user type string
ASSERT $value != NONE 
    AND is::email($value);

define field password on user type string assert $value != none;

create user set email = 'mufassir73@student.sust.edu', password = crypto::argon2::generate("root");

relate (select id from user where email = 'mufassir73@student.sust.edu') -> login -> student:2019331073;

create user set email = 'asanul13@student.sust.edu', password = crypto::argon2::generate("neel");

relate (select id from user where email = 'asanul13@student.sust.edu') -> login -> student:2019331013;

create user set email = 'mufassir73@sust.edu', password = crypto::argon2::generate("root");

relate (select id from user where email = 'mufassir73@sust.edu') -> login -> teacher:2019331073;

create user set email = 'xyz@sust.edu', password = crypto::argon2::generate("root");

relate (select id from user where email = 'xyz@sust.edu') -> login -> teacher:2016331033;

create user set email = 'mufassir@sust.edu', password = crypto::argon2::generate("root");

relate (select id from user where email = 'mufassir@sust.edu') -> login -> admin:2019331002;

create user set email = 'mrx@sust.edu', password = crypto::argon2::generate("root");

relate (select id from user where email = 'mrx@sust.edu') -> login -> admin:2019331004;

Define scope teacher session 24h
signin(select * from user where email = $email and password = crypto::argon2::compare(password, $pass));

select ->login->teacher.id as id
from user
where email = $email and password = crypto::argon2::compare(password, $pass) split id;


Define scope admin session 24h
signin(select * from user where email = $email and password = crypto::argon2::compare(password, $pass));

Define scope student session 24h
signin(select * from user where email = $email and password = crypto::argon2::compare(password, $pass));






```