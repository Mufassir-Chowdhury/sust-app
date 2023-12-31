```sql
surreal start --log debug --user root --pass root memory

surreal import --conn http://localhost:8000 --user root --pass root --ns test --db test export.surql
```

```sql
DEFINE TABLE department SCHEMAFULL;

DEFINE FIELD code ON department TYPE number 
    ASSERT $value != NONE AND $value >= 100 AND $value <= 999 
    AND math::round($value) = $value;
DEFINE INDEX code ON department FIELDS code UNIQUE;

DEFINE FIELD letter_code ON department TYPE string 
    ASSERT $value != NONE AND string::len($value) = 3 AND $value = /[A-Z]{3}/;
DEFINE INDEX letter_code ON department FIELDS letter_code UNIQUE;

DEFINE FIELD name ON department TYPE string 
    ASSERT $value != NONE AND $value = /^[A-Za-z ]+$/
    AND string::len($value) > 5;
DEFINE INDEX name ON department FIELDS name UNIQUE;

DEFINE FIELD building ON department TYPE string 
    ASSERT $value != NONE AND string::len($value) > 0;

DEFINE FIELD floor ON department TYPE number 
    ASSERT $value != NONE AND $value > 0 AND $value <= 10 AND 
    math::round($value) = $value;

DEFINE FIELD minor_course_code ON department TYPE string 
    ASSERT $value != NONE AND $value = /[A-Z]{1}/ 
    AND string::len($value) == 1;
DEFINE INDEX minor_course_code ON department FIELDS minor_course_code UNIQUE;

create department content {
    code : 331,
    id : string::trim('CSE'),
    letter_code : string::trim('CSE'),
    name : string::trim('Computer Science and Engineering'),
    building : string::trim('Dr. M. A. Wazed Miah IICT Building'),
    floor : 3,
    minor_course_code : string::trim('D'),
};

create department content {
    code : 123,
    id : string::trim('SWE'),
    letter_code : string::trim('SWE'),
    name : string::trim('Software Engineering'),
    building : string::trim('Dr. M. A. Wazed Miah IICT Building'),
    floor : 1,
    minor_course_code : string::trim('A'),
};

create department content {
    code : 330,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim('B'),
};

create department content {
    code : 132,
    id : string::trim('PHY'),
    letter_code : string::trim('PHY'),
    name : string::trim('Physics'),
    building : string::trim('A'),
    floor : 1,
    minor_course_code : string::trim('C'),
};


DEFINE table student SCHEMAFULL;

DEFINE FIELD id ON student TYPE string;

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

DEFINE FIELD personal.phone ON student TYPE string;

DEFINE INDEX phone ON student FIELDS personal.phone UNIQUE;

DEFINE FIELD personal.hometown ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD result ON student TYPE object;

DEFINE FIELD result.cgpa ON student TYPE float 
    ASSERT $value == NONE OR $value >= 0 AND $value <= 4;

DEFINE FIELD result.grade ON student TYPE string 
    ASSERT $value == NONE OR $value INSIDE ['A+', 'A', 'A-', 'B+', 'B', 'B-', 'C+', 'C', 'C-', 'F'];

DEFINE FIELD result.total_credit ON student TYPE float 
    ASSERT $value == NONE OR $value >= 0 AND $value <= 160;

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
        phone: string::trim('01771144308'),
        hometown: string::trim('Sylhet'),
    },
    result: {
        cgpa: 4,
        grade: string::trim('A+'),
        total_credit: 160,
    },
    
};

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
        privilege: null,

    personal: {
        father: string::trim('Md Mojammel Haque'),
        mother: string::trim('Most Ayasha Siddika'),
        birthday: "2000-12-03T07:18:52Z",
        phone: string::trim('01771144309'),
        hometown: string::trim('Naogaon'),
    },
    result: {
        cgpa: 3.94,
        grade: string::trim('A'),
        total_credit: 160,
    },
    
};





DEFINE TABLE teacher SCHEMAFULL;

DEFINE FIELD id ON teacher TYPE string;

DEFINE FIELD name ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z .]+$/ 
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
    AND $value = /^[A-Za-z .]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD personal.mother ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z .]+$/ 
    AND string::len($value) >= 3;

--TODO
DEFINE FIELD personal.birthday ON teacher TYPE datetime 
    ASSERT $value != NONE AND 
    time::year($value) < time::year()-15;

DEFINE FIELD personal.phone ON teacher TYPE string;

DEFINE INDEX phone ON teacher FIELDS personal.phone UNIQUE;

DEFINE FIELD personal.hometown ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3;  


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

CREATE teacher:2016331033 CONTENT {
    name: string::trim('Mr. X'),
    department: department:CSE,
    designation : string::trim('Lecturer'),
    email: {
       personal: string::trim('xyzz@gmail.com'),
       academic:string::trim( 'xyz@sust.edu'),
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





DEFINE TABLE admin SCHEMAFULL;
DEFINE FIELD id ON admin TYPE string;

DEFINE FIELD name ON admin TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD department ON admin TYPE record(department) 
    ASSERT $value != NONE;

define field designation on admin type string
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD email ON admin TYPE object;

DEFINE FIELD email.personal ON admin TYPE string 
    ASSERT $value != NONE 
    AND is::email($value);

DEFINE INDEX personal_email ON admin FIELDS email.personal UNIQUE;

DEFINE FIELD email.academic ON admin TYPE string 
    ASSERT $value != NONE AND is::email($value) 
    AND parse::email::host($value) = 'sust.edu' 
    AND parse::email::user($value) >= 3;

DEFINE INDEX academic_email ON admin FIELDS email.academic UNIQUE;

DEFINE FIELD gender ON admin TYPE string 
    ASSERT $value INSIDE ['male', 'female'];

DEFINE FIELD blood_group ON admin TYPE string 
    ASSERT $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-'];

DEFINE FIELD personal ON admin TYPE object;

DEFINE FIELD personal.father ON admin TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

DEFINE FIELD personal.mother ON admin TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3;

--TODO
DEFINE FIELD personal.birthday ON admin TYPE datetime 
    ASSERT $value != NONE AND 
    time::year($value) < time::year()-15;

DEFINE FIELD personal.phone ON admin TYPE string ;

DEFINE INDEX phone ON admin FIELDS personal.phone UNIQUE;

DEFINE FIELD personal.hometown ON admin TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3;


CREATE admin:2019331004 CONTENT {
    name: string::trim('Mr. X'),
    department: department:CSE,
	designation: string::trim("Assistant Something"),
    email: {
       personal: string::trim('mac22214u@gmail.com'),
       academic: string::trim('mrx@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Mr. X'),
        mother: string::trim('Mrs. Y'),
        birthday: "2001-07-10T07:18:52Z",
        phone: "01771144308",
        hometown: string::trim('Sylhet'),
    },
};

CREATE admin:2019331002 CONTENT {
    name: string::trim('Mr. Xx'),
    department: department:CSE,
	designation: string::trim("Assistant Something"),
    email: {
       personal: string::trim('mac2221@gmail.com'),
       academic: string::trim('mufassir@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Mr. X'),
        mother: string::trim('Mrs. Y'),
        birthday: "2001-07-10T07:18:52Z",
        phone: "01771144309",
        hometown: string::trim('Sylhet'),
    },
};



DEFINE TABLE course SCHEMAFUL;

define field course_code on course type string
    value meta::id(id);

define field name on course type string
    assert $value != none
    and $value = /^[A-Za-z ]+$/
    and string::len($value) > 5;

define index name on course fields name unique;

define field credit on course type float
    assert $value != none
    and $value > 0 and $value <= 4;

define field type on course type string
    assert $value inside ['Lab', 'Theory', 'Thesis'];

DEFINE FIELD department ON course TYPE record(department)
    assert $value != none;

define field syllabus on course type array;
define field syllabus.* on course type object;

define field syllabus.*.title on course type string 
    assert $value != none
    and $value = /^[A-Za-z ,&0-9]+$/
    and string::len($value) > 5;

define field syllabus.*.topics on course type array;
define field syllabus.*.topics.* on course type string
    assert $value != none
    and string::len($value) > 5;

CREATE course:CSE222 CONTENT {
	credit: 3,
	name: 'Computer Science',
	department: department:CSE,
	type: 'Theory',
	syllabus: [
		{
			title: 'Topic 1',
			topics: [
				'Discussion',
				'Something other than that',
			],
		},
		{
			title: 'Topic 2',
			topics: [
				'Discussion2',
				'Something other than that',
			],
		},
	],
};

CREATE course:CSE330 CONTENT {
	credit: 2,
	name: 'Web Tech',
	department: department:CSE,
	type: 'Lab',
	syllabus: [
		{
			title: 'Topic 1',
			topics: [
				'Discussion',
				'Something other than that',
			],
		},
		{
			title: 'Topic 2',
			topics: [
				'Discussion2',
				'Something other than that',
			],
		},
	],
}

DEFINE SCOPE teacher SESSION 1d SIGNIN (SELECT * FROM teacher WHERE email = $username);
DEFINE SCOPE student SESSION 1d SIGNIN (SELECT * FROM student WHERE email.academic = $username);
DEFINE SCOPE admin SESSION 1d SIGNIN (SELECT * FROM admin WHERE email = $username);

```

``` sql
select ->(offers where semester=12 and year = )->course from department
```