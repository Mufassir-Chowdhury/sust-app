### To create the table `student`
```sql
DEFINE table student SCHEMAFULL;

/*define field id on student type number
    assert $value != none 
    and string::len(<string>$value) = 10 and 
    string::startsWith(<string>$value, '20') = true; */

DEFINE FIELD name ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD department ON student TYPE record(department) 
    ASSERT $value != NONE;

DEFINE FIELD email ON student TYPE object;

DEFINE FIELD email.personal ON student TYPE string 
    ASSERT $value != NONE 
    AND is::email($value);

DEFINE INDEX personal_email ON student FIELDS email.personal UNIQUE;

--TODO
DEFINE FIELD email.academic ON student TYPE string 
    ASSERT $value != NONE AND is::email($value) 
    AND parse::email::host($value) = 'student.sust.edu' 
    AND parse::email::user($value) >= 5 
    AND is::numeric(string::slice(parse::email::user($value), string::len(parse::email::user($value)) - 2, 2)) 
    AND is::alpha(string::slice(parse::email::user($value), 0, string::len(parse::email::user($value)) - 2));

DEFINE INDEX academic_email ON student FIELDS email.academic UNIQUE;

DEFINE FIELD gender ON student TYPE string 
    ASSERT $value INSIDE ['male', 'female'];

DEFINE FIELD session ON student TYPE number 
    ASSERT $value != NONE 
    AND $value >= 1980 AND $value <= time::year() 
    AND math::round($value) = $value;

DEFINE FIELD current_semester ON student TYPE number 
    ASSERT $value != NONE 
    AND $value >= 1 AND $value <= 8 
    AND math::round($value) = $value;

DEFINE FIELD blood_group ON student TYPE string 
    ASSERT $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-'];

DEFINE FIELD privilege ON student TYPE string 
    ASSERT $value = NONE 
    OR ($value = /^[A-Za-z. ]+$/ 
    AND string::len($value) > 1);

DEFINE FIELD personal ON student TYPE object;

DEFINE FIELD personal.father ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD personal.mother ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

--TODO
DEFINE FIELD personal.birthday ON student TYPE datetime 
    ASSERT $value != NONE AND 
    time::year($value) < time::year()-15;

DEFINE FIELD personal.phone ON student TYPE number 
    ASSERT $value != NONE 
    AND math::round($value) = $value 
    AND string::startsWith(<string> $value, '1') 
    AND string::len(<string> $value) = 10;

DEFINE INDEX phone ON student FIELDS personal.phone UNIQUE;

DEFINE FIELD personal.hometown ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD result ON student TYPE object;

DEFINE FIELD result.cgpa ON student TYPE float 
    ASSERT $value != NONE 
    AND $value >= 0 AND $value <= 4;

DEFINE FIELD result.grade ON student TYPE string 
    ASSERT $value INSIDE ['A+', 'A', 'A-', 'B+', 'B', 'B-', 'C+', 'C', 'C-', 'F'];

DEFINE FIELD result.total_credit ON student TYPE float 
    ASSERT $value != NONE 
    AND $value >= 0 AND $value <= 160;

```

### `student` adding format
```sql
CREATE student:$value(id) CONTENT {
    name: string::trim($value (string)),
    department: string::concat('department:', $value(string)),
    email: {
       personal: string::trim($value (string)),
       academic:string::trim($value (string)),
    },
    gender: string::lowercase($value (string)),
    session: $value (number),
    current_semester: $value (number),
    blood_group: string::trim($value (string)),
    privilege: string::trim($value (string)),
    personal: {
        father: string::trim($value (string)),
        mother: string::trim($value (string)),
        birthday: $value (datetime),
        phone: $value (number),
        hometown: string::trim($value (string)),
    },
    result: {
        cgpa: $value (float),
        grade: string::trim($value (string)),
        total_credit: $value (float),
    },  
};
create user set email = string::trim($value (string)), password = crypto::argon2::generate($value(string));
relate (select id from user where email = string::trim($value (string))) -> login -> student:$value(id);

```

### Example to insert some values
```sql

CREATE student:2019331073 CONTENT {
    name: string::trim('Mufassir Ahmad Chowdhury'),
    department: department:CSE,
    email: {
       personal: string::trim('mac22214u@gmail.com'),
       academic:string::trim( 'mufassir73@student.sust.edu'),
    },
    gender: string::lowercase('male'),
    session: 2019,
    current_semester: 5,
    blood_group: string::trim('B+'),
    privilege: string::trim('CR'),
    personal: {
        father: string::trim('Hafiz Md Mashhud Chowdhury'),
        mother: string::trim('Afsana Begum'),
        birthday: "2001-07-10T07:18:52Z",
        phone: 01771144308,
        hometown: string::trim('Sylhet'),
    },
    result: {
        cgpa: 4,
        grade: string::trim('A+'),
        total_credit: 160,
    },
};
create user set email = 'mufassir73@student.sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'mufassir73@student.sust.edu') -> login -> student:2019331073;


CREATE student:2019331013 CONTENT {
    name: string::trim('Asanul Haque Kiron'),
    department: department:CSE,
    email: {
       personal: string::trim('asanulhaquekiron@gmail.com'),
       academic:string::trim( 'asanul13@student.sust.edu'),
    },
    gender: string::lowercase('male'),
    session: 2019,
    current_semester: 5,
    blood_group: string::trim('A+'),
    personal: {
        father: string::trim('Md Mojammel Haque'),
        mother: string::trim('Most Ayasha Siddika'),
        birthday: "2000-12-03T07:18:52Z",
        phone: 01771144309,
        hometown: string::trim('Naogaon'),
    },
    result: {
        cgpa: 3.94,
        grade: string::trim('A'),
        total_credit: 160,
    },
};
create user set email = 'asanul13@student.sust.edu', password = crypto::argon2::generate("neel");
relate (select id from user where email = 'asanul13@student.sust.edu') -> login -> student:2019331013;

```

### getting name, department name, registration of all students
```sql
select name, department.name, id 
from student;
```

### getting all information of a specific student
```sql
select * from student:id fetch department;
```

### getting metadata using id
```sql
select id, name, department.name, semester 
from student:id; 
```

### getting all information using id
```sql
select *, department.* from student:id;
```

### getting people of the same department
```sql
select id,name
from student
where department = (select value department from student:id);
```

### getting people of the same batch
```sql
select id from student 
where session = (select value session from student.id);
```

### getting people of the same department and batch
```sql
select id from student
where [session, department] = [(select value session from student:2019331013),(select value department from student:2019331013)] 
```