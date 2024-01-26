-- ------------------------------
-- OPTION
-- ------------------------------

OPTION IMPORT;

-- ------------------------------
-- SCOPES
-- ------------------------------

DEFINE SCOPE admin SESSION 1d SIGNIN (SELECT ->login->admin.id AS id FROM user WHERE email = $email AND crypto::argon2::compare(password, $password) SPLIT ON id);
DEFINE SCOPE student SESSION 1d SIGNIN (SELECT ->login->student.id AS id FROM user WHERE email = $email AND crypto::argon2::compare(password, $password) SPLIT ON id);
DEFINE SCOPE teacher SESSION 1d SIGNIN (SELECT ->login->teacher.id AS id FROM user WHERE email = $email AND crypto::argon2::compare(password, $password) SPLIT ON id);

-- ------------------------------
-- TABLE: admin
-- ------------------------------

DEFINE TABLE admin SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD blood_group ON admin TYPE string ASSERT $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-'] PERMISSIONS FULL;
DEFINE FIELD department ON admin TYPE record<department> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD designation ON admin TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD email ON admin TYPE object PERMISSIONS FULL;
DEFINE FIELD email.academic ON admin TYPE string ASSERT $value != NONE AND parse::email::host($value) = 'sust.edu' AND parse::email::user($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD email.personal ON admin TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD gender ON admin TYPE string ASSERT $value INSIDE ['male', 'female'] PERMISSIONS FULL;
DEFINE FIELD name ON admin TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal ON admin TYPE object PERMISSIONS FULL;
DEFINE FIELD personal.birthday ON admin TYPE datetime ASSERT $value != NONE AND time::year($value) < time::year() - 15 PERMISSIONS FULL;
DEFINE FIELD personal.father ON admin TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.hometown ON admin TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z, ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.mother ON admin TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.phone ON admin TYPE string PERMISSIONS FULL;

DEFINE INDEX academic_email ON admin FIELDS email.academic UNIQUE;
DEFINE INDEX personal_email ON admin FIELDS email.personal UNIQUE;
DEFINE INDEX phone ON admin FIELDS personal.phone UNIQUE;

-- ------------------------------
-- TABLE: course
-- ------------------------------

DEFINE TABLE course SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD course_code ON course TYPE string VALUE meta::id(id) PERMISSIONS FULL;
DEFINE FIELD credit ON course TYPE float ASSERT $value != NONE AND $value > 0 AND $value <= 4 PERMISSIONS FULL;
DEFINE FIELD department ON course TYPE record<department> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD name ON course TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z ]+$/ AND string::len($value) > 5 PERMISSIONS FULL;
DEFINE FIELD type ON course TYPE string ASSERT $value INSIDE ['Lab', 'Theory', 'Thesis'] PERMISSIONS FULL;

DEFINE INDEX name ON course FIELDS name UNIQUE;

-- ------------------------------
-- TABLE: dailyroutine
-- ------------------------------

DEFINE TABLE dailyroutine SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD building ON dailyroutine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD date ON dailyroutine TYPE datetime ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD duration ON dailyroutine TYPE string ASSERT $value INSIDE ['1hr', '2hr', 3hr] PERMISSIONS FULL;
DEFINE FIELD room ON dailyroutine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD time ON dailyroutine TYPE number ASSERT $value != NONE AND $value >= 8 AND $value <= 18 PERMISSIONS FULL;

-- ------------------------------
-- TABLE: department
-- ------------------------------

DEFINE TABLE department SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD building ON department TYPE string ASSERT $value != NONE AND string::len($value) > 0 PERMISSIONS FULL;
DEFINE FIELD code ON department TYPE number ASSERT $value != NONE AND $value >= 100 AND $value <= 999 AND math::round($value) = $value PERMISSIONS FULL;
DEFINE FIELD floor ON department TYPE number ASSERT $value != NONE AND $value > 0 AND $value <= 10 AND math::round($value) = $value PERMISSIONS FULL;
DEFINE FIELD letter_code ON department TYPE string ASSERT $value != NONE AND string::len($value) = 3 AND $value = /[A-Z]{3}/ PERMISSIONS FULL;
DEFINE FIELD minor_course_code ON department TYPE string ASSERT $value != NONE AND $value = /[A-Z]{1}/ AND string::len($value) == 1 PERMISSIONS FULL;
DEFINE FIELD name ON department TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z ]+$/ AND string::len($value) > 5 PERMISSIONS FULL;

DEFINE INDEX code ON department FIELDS code UNIQUE;
DEFINE INDEX letter_code ON department FIELDS letter_code UNIQUE;
DEFINE INDEX minor_course_code ON department FIELDS minor_course_code UNIQUE;
DEFINE INDEX name ON department FIELDS name UNIQUE;

-- ------------------------------
-- TABLE: follows
-- ------------------------------

DEFINE TABLE follows SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: has
-- ------------------------------

DEFINE TABLE has SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: having
-- ------------------------------

DEFINE TABLE having SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: login
-- ------------------------------

DEFINE TABLE login SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: offers
-- ------------------------------

DEFINE TABLE offers SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: post
-- ------------------------------

DEFINE TABLE post SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD description ON post TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD info ON post TYPE object PERMISSIONS FULL;
DEFINE FIELD info.date ON post TYPE datetime ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD info.posted_by ON post TYPE record<teacher> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD info.title ON post TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD pdf ON post TYPE string PERMISSIONS FULL;

-- ------------------------------
-- TABLE: resource
-- ------------------------------

DEFINE TABLE resource SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD date ON resource TYPE datetime ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD link ON resource TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD posted_by ON resource TYPE record<student | admin | teacher> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD title ON resource TYPE string ASSERT $value != NONE PERMISSIONS FULL;

-- ------------------------------
-- TABLE: routine
-- ------------------------------

DEFINE TABLE routine SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD building ON routine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD duration ON routine TYPE string ASSERT $value INSIDE ['1hr', '2hr', 3hr] PERMISSIONS FULL;
DEFINE FIELD room ON routine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD time ON routine TYPE number ASSERT $value != NONE AND $value >= 8 AND $value <= 17 PERMISSIONS FULL;

-- ------------------------------
-- TABLE: student
-- ------------------------------

DEFINE TABLE student SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD blood_group ON student TYPE string ASSERT $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-'] PERMISSIONS FULL;
DEFINE FIELD current_semester ON student TYPE number ASSERT $value != NONE AND $value >= 1 AND $value <= 8 AND math::round($value) = $value PERMISSIONS FULL;
DEFINE FIELD department ON student TYPE record<department> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD email ON student TYPE object PERMISSIONS FULL;
DEFINE FIELD email.academic ON student TYPE string ASSERT $value != NONE AND parse::email::host($value) = 'student.sust.edu' AND parse::email::user($value) >= 5 PERMISSIONS FULL;
DEFINE FIELD email.personal ON student TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD gender ON student TYPE string ASSERT $value INSIDE ['male', 'female'] PERMISSIONS FULL;
DEFINE FIELD name ON student TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal ON student TYPE object PERMISSIONS FULL;
DEFINE FIELD personal.birthday ON student TYPE datetime ASSERT $value != NONE AND time::year($value) < time::year() - 15 PERMISSIONS FULL;
DEFINE FIELD personal.father ON student TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.hometown ON student TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z, ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.mother ON student TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.phone ON student TYPE string PERMISSIONS FULL;
DEFINE FIELD privilage ON student TYPE string ASSERT $value = NONE OR ($value = /^[A-Za-z. ]+$/ AND string::len($value) > 1) PERMISSIONS FULL;
DEFINE FIELD session ON student TYPE number ASSERT $value != NONE AND $value >= 1980 AND $value <= time::year() AND math::round($value) = $value PERMISSIONS FULL;

DEFINE INDEX academic_email ON student FIELDS email.academic UNIQUE;
DEFINE INDEX personal_email ON student FIELDS email.personal UNIQUE;
DEFINE INDEX phone ON student FIELDS personal.phone UNIQUE;

-- ------------------------------
-- TABLE: takes
-- ------------------------------

DEFINE TABLE takes SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: teacher
-- ------------------------------

DEFINE TABLE teacher SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD blood_group ON teacher TYPE string ASSERT $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-'] PERMISSIONS FULL;
DEFINE FIELD department ON teacher TYPE record<department> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD designation ON teacher TYPE string ASSERT $value INSIDE ['Lecturer', 'Assistant Professor', 'Professor'] PERMISSIONS FULL;
DEFINE FIELD email ON teacher TYPE object PERMISSIONS FULL;
DEFINE FIELD email.academic ON teacher TYPE string ASSERT $value != NONE AND parse::email::host($value) = 'sust.edu' AND parse::email::user($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD email.personal ON teacher TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD gender ON teacher TYPE string ASSERT $value INSIDE ['male', 'female'] PERMISSIONS FULL;
DEFINE FIELD name ON teacher TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z .]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal ON teacher TYPE object PERMISSIONS FULL;
DEFINE FIELD personal.birthday ON teacher TYPE datetime ASSERT $value != NONE AND time::year($value) < time::year() - 15 PERMISSIONS FULL;
DEFINE FIELD personal.father ON teacher TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z .]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.hometown ON teacher TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z, ]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.mother ON teacher TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z .]+$/ AND string::len($value) >= 3 PERMISSIONS FULL;
DEFINE FIELD personal.phone ON teacher TYPE string PERMISSIONS FULL;

DEFINE INDEX academic_email ON teacher FIELDS email.academic UNIQUE;
DEFINE INDEX personal_email ON teacher FIELDS email.personal UNIQUE;
DEFINE INDEX phone ON teacher FIELDS personal.phone UNIQUE;

-- ------------------------------
-- TABLE: teaches
-- ------------------------------

DEFINE TABLE teaches SCHEMALESS PERMISSIONS NONE;

-- ------------------------------
-- TABLE: user
-- ------------------------------

DEFINE TABLE user SCHEMAFULL PERMISSIONS NONE;

DEFINE FIELD email ON user TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD password ON user TYPE string ASSERT $value != NONE PERMISSIONS FULL;

DEFINE INDEX email ON user FIELDS email UNIQUE;

-- ------------------------------
-- TRANSACTION
-- ------------------------------

BEGIN TRANSACTION;

-- ------------------------------
-- TABLE DATA: admin
-- ------------------------------

UPDATE admin:2019331004 CONTENT { blood_group: 'B+', department: department:CSE, designation: 'Assistant Something', email: { academic: 'mrx@sust.edu', personal: 'mac22214u@gmail.com' }, gender: 'male', id: admin:2019331004, name: 'Mr. X', personal: { birthday: '2001-07-10T07:18:52Z', father: 'Mr. X', hometown: 'Sylhet', mother: 'Mrs. Y', phone: '01771144308' } };

-- ------------------------------
-- TABLE DATA: course
-- ------------------------------

UPDATE course:CSE222 CONTENT { course_code: 'CSE222', credit: 3, department: department:CSE, id: course:CSE222, name: 'Computer Science', type: 'Theory' };
UPDATE course:CSE330 CONTENT { course_code: 'CSE330', credit: 2, department: department:CSE, id: course:CSE330, name: 'Web Tech', type: 'Lab' };

-- ------------------------------
-- TABLE DATA: dailyroutine
-- ------------------------------

UPDATE dailyroutine:ndeb82uxfmwgybsuk2md CONTENT { building: 'IICT', date: '2023-08-24T19:32:45.564948Z', duration: '1hr', id: dailyroutine:ndeb82uxfmwgybsuk2md, room: 'G-2', time: 14 };

-- ------------------------------
-- TABLE DATA: department
-- ------------------------------

UPDATE department:CSE CONTENT { building: 'Dr. M. A. Wazed Miah IICT Building', code: 331, floor: 3, id: department:CSE, letter_code: 'CSE', minor_course_code: 'D', name: 'Computer Science and Engineering' };
UPDATE department:ECO CONTENT { building: 'D', code: 330, floor: 3, id: department:ECO, letter_code: 'ECO', minor_course_code: 'B', name: 'Economics' };
UPDATE department:EEE CONTENT { building: 'Dr. M. A. Wazed Miah IICT Building', code: 391, floor: 4, id: department:EEE, letter_code: 'EEE', minor_course_code: 'E', name: 'Electrical and Electronics Engineering' };
UPDATE department:PHY CONTENT { building: 'A', code: 132, floor: 1, id: department:PHY, letter_code: 'PHY', minor_course_code: 'C', name: 'Physics' };
UPDATE department:SWE CONTENT { building: 'Dr. M. A. Wazed Miah IICT Building', code: 123, floor: 1, id: department:SWE, letter_code: 'SWE', minor_course_code: 'A', name: 'Software Engineering' };

-- ------------------------------
-- TABLE DATA: follows
-- ------------------------------

RELATE course:CSE330 -> follows:13wrgnxgn193h41lex3a -> routine:196j2rikt9dgikhpuelv CONTENT { __: true, day: 'Wednesday', id: follows:13wrgnxgn193h41lex3a, in: course:CSE330, out: routine:196j2rikt9dgikhpuelv, year: '2023' };
RELATE course:CSE222 -> follows:4gyn6go2nt1z45jw91bc -> routine:0890gz6xvnjc5oc9cgfc CONTENT { __: true, day: 'Monday', id: follows:4gyn6go2nt1z45jw91bc, in: course:CSE222, out: routine:0890gz6xvnjc5oc9cgfc, year: '2023' };
RELATE course:CSE222 -> follows:5r64y6ulhrhdf8zapyax -> routine:0890gz6xvnjc5oc9cgfc CONTENT { __: true, day: 'Sunday', id: follows:5r64y6ulhrhdf8zapyax, in: course:CSE222, out: routine:0890gz6xvnjc5oc9cgfc, year: '2023' };

-- ------------------------------
-- TABLE DATA: has
-- ------------------------------

RELATE course:CSE222 -> has:gdwh0a4n59m0wa8wukzk -> resource:a2lphbzcgp5ki4xe5jtv CONTENT { __: true, id: has:gdwh0a4n59m0wa8wukzk, in: course:CSE222, out: resource:a2lphbzcgp5ki4xe5jtv, type: 'Video', year: '2023' };
RELATE course:CSE222 -> has:rxpwnox0962wrglsgb3m -> resource:5072z1qsmtzt96uave4x CONTENT { __: true, id: has:rxpwnox0962wrglsgb3m, in: course:CSE222, out: resource:5072z1qsmtzt96uave4x, type: 'Video', year: '2023' };
RELATE course:CSE222 -> has:tzzuw2ajfnjiakvlet0s -> resource:7cjcpi6sqgr8pi1typmx CONTENT { __: true, id: has:tzzuw2ajfnjiakvlet0s, in: course:CSE222, out: resource:7cjcpi6sqgr8pi1typmx, type: 'Pdf', year: '2023' };
RELATE course:CSE222 -> has:ypkepaluk8sy26hugxh2 -> post:vuc687jrwp2014xl18mf CONTENT { __: true, id: has:ypkepaluk8sy26hugxh2, in: course:CSE222, out: post:vuc687jrwp2014xl18mf, year: '2023' };

-- ------------------------------
-- TABLE DATA: having
-- ------------------------------

RELATE course:CSE330 -> having:syn88tvte55ce3zxzvv8 -> dailyroutine:ndeb82uxfmwgybsuk2md CONTENT { __: true, id: having:syn88tvte55ce3zxzvv8, in: course:CSE330, out: dailyroutine:ndeb82uxfmwgybsuk2md };

-- ------------------------------
-- TABLE DATA: login
-- ------------------------------

RELATE user:emfrsjin5aql5nbu2cnz -> login:0rmjsdqk2j0v0qq2thwv -> student:2023119144 CONTENT { __: true, id: login:0rmjsdqk2j0v0qq2thwv, in: user:emfrsjin5aql5nbu2cnz, out: student:2023119144 };
RELATE user:ws6uynksgs1kdr07b3r8 -> login:1sjkmi7o2a9f98a978gi -> teacher:2019331073 CONTENT { __: true, id: login:1sjkmi7o2a9f98a978gi, in: user:ws6uynksgs1kdr07b3r8, out: teacher:2019331073 };
RELATE user:xz5o4prrbak776nivu4a -> login:2or2pea30ymoa6ko5905 -> student:2019331013 CONTENT { __: true, id: login:2or2pea30ymoa6ko5905, in: user:xz5o4prrbak776nivu4a, out: student:2019331013 };
RELATE user:4p3m8nugy0lspbz2mxr8 -> login:74aiw92bacx0k2zvodbu -> student:2022998101 CONTENT { __: true, id: login:74aiw92bacx0k2zvodbu, in: user:4p3m8nugy0lspbz2mxr8, out: student:2022998101 };
RELATE user:pi75cobhfzu0xp5ex832 -> login:ad56liuqu1nfkj275cz5 -> student:2022887055 CONTENT { __: true, id: login:ad56liuqu1nfkj275cz5, in: user:pi75cobhfzu0xp5ex832, out: student:2022887055 };
RELATE user:emfrsjin5aql5nbu2cnz -> login:fc5fhrxvg2lqqfouq513 -> student:2023119144 CONTENT { __: true, id: login:fc5fhrxvg2lqqfouq513, in: user:emfrsjin5aql5nbu2cnz, out: student:2023119144 };
RELATE user:4p3m8nugy0lspbz2mxr8 -> login:pupihh1qmjmigs8dcjky -> student:2022998101 CONTENT { __: true, id: login:pupihh1qmjmigs8dcjky, in: user:4p3m8nugy0lspbz2mxr8, out: student:2022998101 };
RELATE user:hb4sfhwnejl13tkzhfpw -> login:s1ut92rd7ukzly37hbvr -> admin:2019331004 CONTENT { __: true, id: login:s1ut92rd7ukzly37hbvr, in: user:hb4sfhwnejl13tkzhfpw, out: admin:2019331004 };

-- ------------------------------
-- TABLE DATA: offers
-- ------------------------------

RELATE department:ECO -> offers:1jfzfa9qu33174ouv5nt -> course:CSE222 CONTENT { __: true, id: offers:1jfzfa9qu33174ouv5nt, in: department:ECO, out: course:CSE222, semester: 3, year: 2019 };
RELATE department:CSE -> offers:8nxc8n4apw6oblyng901 -> course:CSE330 CONTENT { __: true, id: offers:8nxc8n4apw6oblyng901, in: department:CSE, out: course:CSE330, semester: 12, year: 2019 };
RELATE department:CSE -> offers:stkue740iyvgmrklrtq7 -> course:CSE330 CONTENT { __: true, id: offers:stkue740iyvgmrklrtq7, in: department:CSE, out: course:CSE330, semester: 12, year: 2023 };
RELATE department:CSE -> offers:t5hazd30yi26x82u5n2l -> course:CSE222 CONTENT { __: true, id: offers:t5hazd30yi26x82u5n2l, in: department:CSE, out: course:CSE222, semester: 12, year: 2019 };

-- ------------------------------
-- TABLE DATA: post
-- ------------------------------

UPDATE post:vuc687jrwp2014xl18mf CONTENT { description: 'this is the first post', id: post:vuc687jrwp2014xl18mf, info: { date: '2023-08-25T15:16:34.956770300Z', posted_by: teacher:2019331073, title: 'First Post' }, pdf: '' };

-- ------------------------------
-- TABLE DATA: resource
-- ------------------------------

UPDATE resource:5072z1qsmtzt96uave4x CONTENT { date: '2023-08-25T14:46:55.308837600Z', id: resource:5072z1qsmtzt96uave4x, link: 'https://www.youtube.com/watch?v=C7WFwgDRStM&ab_channel=Fireship', posted_by: student:2019331013, title: 'Introduction to surrealdb' };
UPDATE resource:7cjcpi6sqgr8pi1typmx CONTENT { date: '2023-08-25T14:39:22.443165700Z', id: resource:7cjcpi6sqgr8pi1typmx, link: 'https://drive.google.com/file/d/1BoOMYnJTvUzBFGwZ7jfGzUgqc7NT5PSe/view?usp=drive_link', posted_by: student:2019331073, title: 'Database class note' };
UPDATE resource:a2lphbzcgp5ki4xe5jtv CONTENT { date: '2023-08-25T14:24:04.359252500Z', id: resource:a2lphbzcgp5ki4xe5jtv, link: 'https://www.youtube.com/watch?v=C7WFwgDRStM&ab_channel=Fireship', posted_by: student:2019331073, title: 'Introduction to surrealdb' };
UPDATE resource:q1bc9ex6a58avvsvldno CONTENT { date: '2023-08-25T14:12:16.412947700Z', id: resource:q1bc9ex6a58avvsvldno, link: 'https://www.youtube.com/watch?v=C7WFwgDRStM&ab_channel=Fireship', posted_by: student:2019331073, title: 'Introduction to surrealdb' };

-- ------------------------------
-- TABLE DATA: routine
-- ------------------------------

UPDATE routine:0890gz6xvnjc5oc9cgfc CONTENT { building: 'IICT', duration: '1hr', id: routine:0890gz6xvnjc5oc9cgfc, room: 'G-2', time: 9 };
UPDATE routine:0sv0qyprii255ccv1vfl CONTENT { building: 'IICT', duration: '2hr', id: routine:0sv0qyprii255ccv1vfl, room: '304', time: 10 };
UPDATE routine:196j2rikt9dgikhpuelv CONTENT { building: 'IICT', duration: '1hr', id: routine:196j2rikt9dgikhpuelv, room: 'G-2', time: 14 };

-- ------------------------------
-- TABLE DATA: student
-- ------------------------------

UPDATE student:2019331013 CONTENT { blood_group: 'A+', current_semester: 5, department: department:CSE, email: { academic: 'asanul13@student.sust.edu', personal: 'asanulhaquekiron@gmail.com' }, gender: 'male', id: student:2019331013, name: 'Asanul Haque Kiron', personal: { birthday: '2000-12-03T07:18:52Z', father: 'Md Mojammel Haque', hometown: 'Naogaon', mother: 'Most Ayasha Siddika', phone: '01771144309' }, session: 2019 };
UPDATE student:2019331073 CONTENT { blood_group: 'B+', current_semester: 7, department: department:CSE, email: { academic: 'mufassir73@student.sust.edu', personal: 'mac22214u@gmail.com' }, gender: 'male', id: student:2019331073, name: 'Mufassir Ahmad Chowdhury', personal: { birthday: '2001-07-09T18:00:00Z', father: 'Hafiz Md. Mashhud Chowdhury', hometown: 'Sylhet', mother: 'Afsana Begum', phone: '+8801771144308' }, privilage: 'Default', session: 2019 };
UPDATE student:2022887055 CONTENT { blood_group: 'A-', current_semester: 2, department: department:ECO, email: { academic: 'sophia03@student.sust.edu', personal: 'sophia.ahmed@example.com' }, gender: 'female', id: student:2022887055, name: 'Sophia Ahmed', personal: { birthday: '2001-09-10T14:00:00Z', father: 'Kamal Ahmed', hometown: 'Chittagong', mother: 'Sara Ahmed', phone: '1754321098' }, session: 2022 };
UPDATE student:2022998101 CONTENT { blood_group: 'B-', current_semester: 4, department: department:SWE, email: { academic: 'arjun04@student.sust.edu', personal: 'arjun.chowdhury@example.com' }, gender: 'male', id: student:2022998101, name: 'Arjun Chowdhury', personal: { birthday: '2001-07-12T09:15:00Z', father: 'Rahim Chowdhury', hometown: 'Rajshahi', mother: 'Nasreen Chowdhury', phone: '1743210989' }, session: 2022 };
UPDATE student:2023119144 CONTENT { blood_group: 'O+', current_semester: 5, department: department:PHY, email: { academic: 'amina05@student.sust.edu', personal: 'amina.islam@example.com' }, gender: 'female', id: student:2023119144, name: 'Amina Islam', personal: { birthday: '2001-10-08T10:00:00Z', father: 'Mohammed Islam', hometown: 'Khulna', mother: 'Fatima Islam', phone: '1732109877' }, session: 2022 };

-- ------------------------------
-- TABLE DATA: takes
-- ------------------------------

RELATE student:2019331013 -> takes:4tnahxcwnlgpj6l634dl -> course:CSE222 CONTENT { __: true, id: takes:4tnahxcwnlgpj6l634dl, in: student:2019331013, out: course:CSE222, semester: 12, year: 2019 };
RELATE student:2019331013 -> takes:cvq6uvnqf51c3vvuws97 -> course:CSE330 CONTENT { __: true, id: takes:cvq6uvnqf51c3vvuws97, in: student:2019331013, out: course:CSE330, semester: 12, year: 2019 };

-- ------------------------------
-- TABLE DATA: teacher
-- ------------------------------

UPDATE teacher:2019331073 CONTENT { blood_group: 'B+', department: department:CSE, designation: 'Lecturer', email: { academic: 'mufassir73@sust.edu', personal: 'mac22214u@gmail.com' }, gender: 'male', id: teacher:2019331073, name: 'Mufassir Ahmad Chowdhury', personal: { birthday: '2001-07-10T07:18:52Z', father: 'Hafiz Md Mashhud Chowdhury', hometown: 'Sylhet', mother: 'Afsana Begum', phone: '1771144308' } };

-- ------------------------------
-- TABLE DATA: teaches
-- ------------------------------

RELATE teacher:2019331073 -> teaches:o94o6fvp390utzmery77 -> offers:1jfzfa9qu33174ouv5nt CONTENT { __: true, id: teaches:o94o6fvp390utzmery77, in: teacher:2019331073, out: offers:1jfzfa9qu33174ouv5nt };
RELATE teacher:2019331073 -> teaches:x700jf9ofic4o8lkfxnv -> offers:t5hazd30yi26x82u5n2l CONTENT { __: true, id: teaches:x700jf9ofic4o8lkfxnv, in: teacher:2019331073, out: offers:t5hazd30yi26x82u5n2l };

-- ------------------------------
-- TABLE DATA: user
-- ------------------------------

UPDATE user:3sx7fe9v6w58gag2b625 CONTENT { email: 'xyz@sust.edu', id: user:3sx7fe9v6w58gag2b625, password: '$argon2id$v=19$m=19456,t=2,p=1$7JLreAVwXh73ALGEEjjtXw$DwpTTl/o2TwlZoooQ+9AC8MqwFCmpiIeGt7T54HU3Jc' };
UPDATE user:4p3m8nugy0lspbz2mxr8 CONTENT { email: 'arjun04@student.sust.edu', id: user:4p3m8nugy0lspbz2mxr8, password: '$argon2id$v=19$m=19456,t=2,p=1$W1PLzntnQnQvHVBESwaonw$mqCDsNwfPF/A7Z0WjbkOUmObpzj6kb+QWQyAtoPi/cI' };
UPDATE user:aasa01kbaknch7ol17w7 CONTENT { email: 'mufassir73@student.sust.edu', id: user:aasa01kbaknch7ol17w7, password: '$argon2id$v=19$m=19456,t=2,p=1$c1fIG/sv0IV5k1UFScZVZg$92aoHf8Y/2wUWTBccYvJ1OmtbwkIAwCkaCksS3+w/b8' };
UPDATE user:d9pu7wrq9qgg9ssojw78 CONTENT { email: 'mufassir@sust.edu', id: user:d9pu7wrq9qgg9ssojw78, password: '$argon2id$v=19$m=19456,t=2,p=1$z9bb3s0hsj/Y4eFgiOYeDw$FvNIPOqc39RLOnm1tf3/oALeopjHBh5u8+kDi/BEuWI' };
UPDATE user:emfrsjin5aql5nbu2cnz CONTENT { email: 'amina05@student.sust.edu', id: user:emfrsjin5aql5nbu2cnz, password: '$argon2id$v=19$m=19456,t=2,p=1$YCAS684IJvTsmkv1vH2CjQ$orcHFgHMZnXhtA8mSKGAGZGEqN4vIN+ynAvvSYFtKWA' };
UPDATE user:hb4sfhwnejl13tkzhfpw CONTENT { email: 'mrx@sust.edu', id: user:hb4sfhwnejl13tkzhfpw, password: '$argon2id$v=19$m=19456,t=2,p=1$mzO2HyrwcGWzE9TxXkIwHA$UM3Ql1K5ekDY0/yVOC+eLMooNt+U1Oi/ULyI3jTVV8k' };
UPDATE user:nbe5cbdnq5fke193hw67 CONTENT { email: 'ethan02@student.sust.edu', id: user:nbe5cbdnq5fke193hw67, password: '$argon2id$v=19$m=19456,t=2,p=1$HkXP0cU6hzquhFhok8pdjg$4zkdcs9U/P36y14VFuIjBcHHZI/RVm/gvOi+J3GcdFQ' };
UPDATE user:pi75cobhfzu0xp5ex832 CONTENT { email: 'sophia03@student.sust.edu', id: user:pi75cobhfzu0xp5ex832, password: '$argon2id$v=19$m=19456,t=2,p=1$Vm/FqMc7u6x/PuctKb3Qmw$OaOC6GaD4LZu7+5x6163OOJ9jxNEo4y6DSSphcwioew' };
UPDATE user:ws6uynksgs1kdr07b3r8 CONTENT { email: 'mufassir73@sust.edu', id: user:ws6uynksgs1kdr07b3r8, password: '$argon2id$v=19$m=19456,t=2,p=1$mPBgfJGt3zXLfBgw7m4+4A$sG5wE599w7moglTaMHc5ERT8nJNg77mNhB61M91R1Qg' };
UPDATE user:xz5o4prrbak776nivu4a CONTENT { email: 'asanul13@student.sust.edu', id: user:xz5o4prrbak776nivu4a, password: '$argon2id$v=19$m=19456,t=2,p=1$bA8wPGQ2nVcvf29yRNdpVg$YlyMWq/crbG+rvpGk4IomFzYf2vnyDs0EtwFWlDXenQ' };
UPDATE user:yycl51qmt5pz4gn798ct CONTENT { email: 'amelia01@student.sust.edu', id: user:yycl51qmt5pz4gn798ct, password: '$argon2id$v=19$m=19456,t=2,p=1$G7HE7CxuzYIEOM4tfTzozQ$syRVMt8PCP9TSC2Ab678gIz+HGs16Ease/YgZQ4Fo1M' };

-- ------------------------------
-- TRANSACTION
-- ------------------------------

COMMIT TRANSACTION;

