### To create the table `teacher`
```sql
DEFINE TABLE teacher SCHEMAFULL;

DEFINE FIELD name ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD department ON teacher TYPE record(department) 
    ASSERT $value != NONE;

define field designation on teacher type string
    assert $value inside ['Lecturer', 'Assistant Professor', 'Professor'];

DEFINE FIELD email ON teacher TYPE object;

DEFINE FIELD email.personal ON teacher TYPE string 
    ASSERT $value != NONE 
    AND is::email($value);

DEFINE INDEX personal_email ON teacher FIELDS email.personal UNIQUE;

DEFINE FIELD email.academic ON teacher TYPE string 
    ASSERT $value != NONE AND is::email($value) 
    AND parse::email::host($value) = 'sust.edu' 
    AND parse::email::user($value) >= 3;

DEFINE INDEX academic_email ON teacher FIELDS email.academic UNIQUE;

DEFINE FIELD gender ON teacher TYPE string 
    ASSERT $value INSIDE ['male', 'female'];

DEFINE FIELD blood_group ON teacher TYPE string 
    ASSERT $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-'];

DEFINE FIELD personal ON teacher TYPE object;

DEFINE FIELD personal.father ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD personal.mother ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

--TODO
DEFINE FIELD personal.birthday ON teacher TYPE datetime 
    ASSERT $value != NONE AND 
    time::year($value) < time::year()-15;

DEFINE FIELD personal.phone ON teacher TYPE number 
    ASSERT $value != NONE 
    AND math::round($value) = $value 
    AND string::startsWith(<string> $value, '1') 
    AND string::len(<string> $value) = 10;

DEFINE INDEX phone ON teacher FIELDS personal.phone UNIQUE;

DEFINE FIELD personal.hometown ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3;  

```

### `teacher` adding format
```sql
CREATE teacher:$value(id) CONTENT {
    name: string::trim($value (string)),
    department: string::concat('department:', $value(string)),
    designation : string::trim($value (string)),
    email: {
       personal: string::trim($value (string)),
       academic:string::trim($value (string)),
    },
    gender: string::lowercase($value (string)),
    blood_group: string::trim($value (string)),
    personal: {
        father: string::trim($value (string)),
        mother: string::trim($value (string)),
        birthday: $value (datetime),
        phone: $value (number),
        hometown: string::trim($value (string)),
    },
};
create user set email = string::trim($value (string)), password = crypto::argon2::generate($value(string));
relate (select id from user where email = string::trim($value (string))) -> login -> teacher:$value(id);
```

### Example to insert some value
```sql
CREATE teacher:2019331073 CONTENT {
    name: string::trim('Mufassir Ahmad Chowdhury'),
    department: department:CSE,
    designation : string::trim('Lecturer'),
    email: {
       personal: string::trim('mac22214u@gmail.com'),
       academic:string::trim( 'mufassir73@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Hafiz Md Mashhud Chowdhury'),
        mother: string::trim('Afsana Begum'),
        birthday: "2001-07-10T07:18:52Z",
        phone: 01771144308,
        hometown: string::trim('Sylhet'),
    },
};
create user set email = 'mufassir73@sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'mufassir73@sust.edu') -> login -> teacher:2019331073;


CREATE teacher:2016331033 CONTENT {
    name: string::trim('Mr. X'),
    department: department:CSE,
    designation : string::trim('Lecturer'),
    email: {
       personal: string::trim('mac22214u@gmail.com'),
       academic:string::trim( 'mufassir73@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Mr. Y'),
        mother: string::trim('Mrs. Z'),
        birthday: "2001-07-10T07:18:52Z",
        phone: 01771144309,
        hometown: string::trim('Khulna'),
    },
};
create user set email = 'mufassir73@sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'mufassir73@sust.edu') -> login -> teacher:2019331073;
```

### getting information of all teacher
```sql
select id, name, department.name
from teacher;
```

### getting all information of a specific teacher
```sql
select * 
from teacher
fetch department;
```


