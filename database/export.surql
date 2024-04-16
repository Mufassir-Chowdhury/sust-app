-- ------------------------------
-- OPTION
-- ------------------------------

OPTION IMPORT;

-- ------------------------------
-- SCOPES
-- ------------------------------

DEFINE SCOPE admin SESSION 1d SIGNUP (CREATE user CONTENT { email: $email, name: $name, password: crypto::argon2::generate($password) }) SIGNIN (SELECT <-login<-admin.id AS id FROM user WHERE email = $email AND crypto::argon2::compare(password, $password) SPLIT ON id);
DEFINE SCOPE student SESSION 1d SIGNUP (CREATE user CONTENT { email: $email, name: $name, password: crypto::argon2::generate($password) }) SIGNIN (SELECT <-login<-student.id AS id FROM user WHERE email = $email AND crypto::argon2::compare(password, $password) SPLIT ON id);
DEFINE SCOPE teacher SESSION 1d SIGNUP (CREATE user CONTENT { email: $email, name: $name, password: crypto::argon2::generate($password) }) SIGNIN (SELECT <-login<-teacher.id AS id FROM user WHERE email = $email AND crypto::argon2::compare(password, $password) SPLIT ON id);

-- ------------------------------
-- TABLE: admin
-- ------------------------------

DEFINE TABLE admin SCHEMAFULL PERMISSIONS FULL;

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
-- TABLE: assignment
-- ------------------------------

DEFINE TABLE assignment SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: class
-- ------------------------------

DEFINE TABLE class SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: course
-- ------------------------------

DEFINE TABLE course SCHEMAFULL PERMISSIONS FULL;

DEFINE FIELD course_code ON course TYPE string VALUE meta::id(id) PERMISSIONS FULL;
DEFINE FIELD credit ON course TYPE float ASSERT $value != NONE AND $value > 0 AND $value <= 4 PERMISSIONS FULL;
DEFINE FIELD department ON course TYPE record<department> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD name ON course TYPE string ASSERT $value != NONE AND $value = /^[A-Za-z ]+$/ AND string::len($value) > 5 PERMISSIONS FULL;
DEFINE FIELD semester ON course TYPE int PERMISSIONS FULL;
DEFINE FIELD type ON course TYPE string ASSERT $value INSIDE ['Lab', 'Theory', 'Thesis'] PERMISSIONS FULL;
DEFINE FIELD year ON course TYPE int PERMISSIONS FULL;

DEFINE INDEX name ON course FIELDS name UNIQUE;

-- ------------------------------
-- TABLE: dailyroutine
-- ------------------------------

DEFINE TABLE dailyroutine SCHEMAFULL PERMISSIONS FULL;

DEFINE FIELD building ON dailyroutine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD date ON dailyroutine TYPE datetime ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD duration ON dailyroutine TYPE string ASSERT $value INSIDE ['1hr', '2hr', 3hr] PERMISSIONS FULL;
DEFINE FIELD room ON dailyroutine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD time ON dailyroutine TYPE number ASSERT $value != NONE AND $value >= 8 AND $value <= 18 PERMISSIONS FULL;

-- ------------------------------
-- TABLE: department
-- ------------------------------

DEFINE TABLE department SCHEMAFULL PERMISSIONS FULL;

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
-- TABLE: exam
-- ------------------------------

DEFINE TABLE exam SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: follows
-- ------------------------------

DEFINE TABLE follows SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: has
-- ------------------------------

DEFINE TABLE has SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: having
-- ------------------------------

DEFINE TABLE having SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: login
-- ------------------------------

DEFINE TABLE login SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: notice
-- ------------------------------

DEFINE TABLE notice SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: notify
-- ------------------------------

DEFINE TABLE notify SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: offers
-- ------------------------------

DEFINE TABLE offers SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: post
-- ------------------------------

DEFINE TABLE post SCHEMAFULL PERMISSIONS FULL;

DEFINE FIELD description ON post TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD info ON post TYPE object PERMISSIONS FULL;
DEFINE FIELD info.date ON post TYPE datetime ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD info.posted_by ON post TYPE record<teacher> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD info.title ON post TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD pdf ON post TYPE string PERMISSIONS FULL;

-- ------------------------------
-- TABLE: resource
-- ------------------------------

DEFINE TABLE resource SCHEMAFULL PERMISSIONS FULL;

DEFINE FIELD date ON resource TYPE datetime ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD link ON resource TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD posted_by ON resource TYPE record<student | admin | teacher> ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD title ON resource TYPE string ASSERT $value != NONE PERMISSIONS FULL;

-- ------------------------------
-- TABLE: routine
-- ------------------------------

DEFINE TABLE routine SCHEMAFULL PERMISSIONS FULL;

DEFINE FIELD building ON routine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD duration ON routine TYPE string ASSERT $value INSIDE ['1hr', '2hr', 3hr] PERMISSIONS FULL;
DEFINE FIELD room ON routine TYPE string ASSERT $value != NONE PERMISSIONS FULL;
DEFINE FIELD time ON routine TYPE number ASSERT $value != NONE AND $value >= 8 AND $value <= 17 PERMISSIONS FULL;

-- ------------------------------
-- TABLE: student
-- ------------------------------

DEFINE TABLE student SCHEMAFULL PERMISSIONS FULL;

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
-- TABLE: taken
-- ------------------------------

DEFINE TABLE taken SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: takes
-- ------------------------------

DEFINE TABLE takes SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: teacher
-- ------------------------------

DEFINE TABLE teacher SCHEMAFULL PERMISSIONS FULL;

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

DEFINE TABLE teaches SCHEMALESS PERMISSIONS FULL;

-- ------------------------------
-- TABLE: user
-- ------------------------------

DEFINE TABLE user SCHEMAFULL PERMISSIONS FULL;

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
UPDATE admin:2022331001 CONTENT { blood_group: 'A+', department: department:CSE, designation: 'Assistant Registrar', email: { academic: 'admin1@sust.edu', personal: 'admin1@example.com' }, gender: 'male', id: admin:2022331001, name: 'John Doe', personal: { birthday: '1985-05-15T00:00:00Z', father: 'Robert Doe', hometown: 'Dhaka', mother: 'Jane Doe', phone: '01711223344' } };
UPDATE admin:2023331003 CONTENT { blood_group: 'AB+', department: department:EEE, designation: 'Administrative Officer', email: { academic: 'admin3@sust.edu', personal: 'admin3@example.com' }, gender: 'male', id: admin:2023331003, name: 'David Lee', personal: { birthday: '1985-09-15T00:00:00Z', father: 'Robert Lee', hometown: 'Chittagong', mother: 'Jessica Lee', phone: '01711334455' } };
UPDATE admin:2024331004 CONTENT { blood_group: 'O+', department: department:ECO, designation: 'Assistant Registrar', email: { academic: 'admin4@sust.edu', personal: 'admin4@example.com' }, gender: 'female', id: admin:2024331004, name: 'Emily Wilson', personal: { birthday: '1990-11-20T00:00:00Z', father: 'Michael Wilson', hometown: 'Sylhet', mother: 'Sarah Wilson', phone: '01711445566' } };
UPDATE admin:2025331005 CONTENT { blood_group: 'A-', department: department:PHY, designation: 'Administrative Officer', email: { academic: 'admin5@sust.edu', personal: 'admin5@example.com' }, gender: 'male', id: admin:2025331005, name: 'William Brown', personal: { birthday: '1982-03-01T00:00:00Z', father: 'Robert Brown', hometown: 'Khulna', mother: 'Elizabeth Brown', phone: '01711556677' } };
UPDATE admin:2026331006 CONTENT { blood_group: 'B-', department: department:CSE, designation: 'Assistant Registrar', email: { academic: 'admin6@sust.edu', personal: 'admin6@example.com' }, gender: 'female', id: admin:2026331006, name: 'Sophia Davis', personal: { birthday: '1988-06-14T00:00:00Z', father: 'Michael Davis', hometown: 'Rajshahi', mother: 'Jessica Davis', phone: '01711667788' } };
UPDATE admin:2027331007 CONTENT { blood_group: 'AB-', department: department:EEE, designation: 'Administrative Officer', email: { academic: 'admin7@sust.edu', personal: 'admin7@example.com' }, gender: 'male', id: admin:2027331007, name: 'Michael Thompson', personal: { birthday: '1985-12-25T00:00:00Z', father: 'Robert Thompson', hometown: 'Barishal', mother: 'Karen Thompson', phone: '01711778899' } };
UPDATE admin:2028331008 CONTENT { blood_group: 'O-', department: department:ECO, designation: 'Assistant Registrar', email: { academic: 'admin8@sust.edu', personal: 'admin8@example.com' }, gender: 'female', id: admin:2028331008, name: 'Emily Garcia', personal: { birthday: '1993-07-18T00:00:00Z', father: 'Michael Garcia', hometown: 'Rangpur', mother: 'Jessica Garcia', phone: '01711889900' } };
UPDATE admin:2029331009 CONTENT { blood_group: 'A+', department: department:PHY, designation: 'Administrative Officer', email: { academic: 'admin9@sust.edu', personal: 'admin9@example.com' }, gender: 'male', id: admin:2029331009, name: 'David Martinez', personal: { birthday: '1987-02-28T00:00:00Z', father: 'Robert Martinez', hometown: 'Cumilla', mother: 'Elizabeth Martinez', phone: '01711990011' } };
UPDATE admin:2030331010 CONTENT { blood_group: 'B+', department: department:CSE, designation: 'Assistant Registrar', email: { academic: 'admin10@sust.edu', personal: 'admin10@example.com' }, gender: 'female', id: admin:2030331010, name: 'Sarah Rodriguez', personal: { birthday: '1986-09-22T00:00:00Z', father: 'Michael Rodriguez', hometown: 'Feni', mother: 'Karen Rodriguez', phone: '01711001122' } };
UPDATE admin:2031331011 CONTENT { blood_group: 'AB+', department: department:EEE, designation: 'Administrative Officer', email: { academic: 'admin11@sust.edu', personal: 'admin11@example.com' }, gender: 'male', id: admin:2031331011, name: 'William Hernandez', personal: { birthday: '1979-04-10T00:00:00Z', father: 'Robert Hernandez', hometown: 'Noakhali', mother: 'Jessica Hernandez', phone: '01711112233' } };

-- ------------------------------
-- TABLE DATA: assignment
-- ------------------------------

UPDATE assignment:yefg4300s6h353zi8e14 CONTENT { creation_date: '2024-04-16', details: 'Details .....asdf', due_date: '2024-04-17', id: assignment:yefg4300s6h353zi8e14, submission: 'on', title: 'Project Submission', total_grade: '100' };
UPDATE assignment:zdq3idbfjm9qaa0vrfrg CONTENT { creation_date: '2024-04-16', details: 'Submission Details', due_date: '2001-01-01', id: assignment:zdq3idbfjm9qaa0vrfrg, submission: 'on', title: 'Another Project Submission', total_grade: '101' };

-- ------------------------------
-- TABLE DATA: class
-- ------------------------------

UPDATE class:36zaynezidr1kehj1xsw CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:36zaynezidr1kehj1xsw };
UPDATE class:5u3rkbkntqobqetfrite CONTENT { attendance: { "student:2019331013": true, "student:2019331073": true }, course: course:CSE222, date: '2024-04-16', id: class:5u3rkbkntqobqetfrite };
UPDATE class:iko3h1d93ue6ykr5648i CONTENT { attendance: { "student:2019331013": true }, course: course:CSE330, date: '2024-04-16', id: class:iko3h1d93ue6ykr5648i };
UPDATE class:j58fg1x9gbo1gm8ye16w CONTENT { attendance: { "student:2019331013": true, "student:2019331073": true }, course: course:CSE222, date: '2024-04-16', id: class:j58fg1x9gbo1gm8ye16w };
UPDATE class:k3j6yxy8lqpuebnkmlp4 CONTENT { attendance: { "student:2019331013": false, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:k3j6yxy8lqpuebnkmlp4 };
UPDATE class:ltmmsqjg2t3mekhs193m CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:ltmmsqjg2t3mekhs193m };
UPDATE class:nnrxdnv23v3xg7izevqm CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:nnrxdnv23v3xg7izevqm };
UPDATE class:plc2xz2k1zf0jj0nx2oy CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false, "student:2020331001": true }, course: course:CSE222, date: '2024-04-16', id: class:plc2xz2k1zf0jj0nx2oy };
UPDATE class:rwtx3u0ypv8i53nco7xa CONTENT { attendance: { "student:2019331013": true, "student:2019331073": true }, course: course:CSE222, date: '2024-04-16', id: class:rwtx3u0ypv8i53nco7xa };
UPDATE class:sy4depxxixiu1nuh0cn5 CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false, "student:2020331001": true }, course: course:CSE222, date: '2024-04-16', id: class:sy4depxxixiu1nuh0cn5 };
UPDATE class:ua5201j43by4z45a9c22 CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:ua5201j43by4z45a9c22 };
UPDATE class:xfzsyw4cm9ykvmc25q4f CONTENT { attendance: { "student:2019331013": false, "student:2019331073": true }, course: course:CSE222, date: '2024-04-16', id: class:xfzsyw4cm9ykvmc25q4f };
UPDATE class:xvnxfywsx06cwiohay7z CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:xvnxfywsx06cwiohay7z };
UPDATE class:zvzvoxzaabiwzrnyefvw CONTENT { attendance: { "student:2019331013": true, "student:2019331073": false }, course: course:CSE222, date: '2024-04-16', id: class:zvzvoxzaabiwzrnyefvw };

-- ------------------------------
-- TABLE DATA: course
-- ------------------------------

UPDATE course:CSE222 CONTENT { course_code: 'CSE222', credit: 3f, department: department:CSE, id: course:CSE222, name: 'Computer Science', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:CSE330 CONTENT { course_code: 'CSE330', credit: 2f, department: department:CSE, id: course:CSE330, name: 'Web Tech', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:CSE333 CONTENT { course_code: 'CSE333', credit: 3f, department: department:CSE, id: course:CSE333, name: 'Software Engineering', semester: 5, type: 'Theory', year: 2019 };
UPDATE course:CSE350 CONTENT { course_code: 'CSE350', credit: 3f, department: department:CSE, id: course:CSE350, name: 'Project Work III', semester: 6, type: 'Lab', year: 2019 };
UPDATE course:CSE431 CONTENT { course_code: 'CSE431', credit: 3f, department: department:CSE, id: course:CSE431, name: 'Computer Networks', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:CSE432 CONTENT { course_code: 'CSE432', credit: 1f, department: department:CSE, id: course:CSE432, name: 'Computer Networks Lab', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:CSE433 CONTENT { course_code: 'CSE433', credit: 3f, department: department:CSE, id: course:CSE433, name: 'Artificial Intelligence', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:CSE434 CONTENT { course_code: 'CSE434', credit: 1f, department: department:CSE, id: course:CSE434, name: 'Artificial Intelligence Lab', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:CSE435 CONTENT { course_code: 'CSE435', credit: 3f, department: department:CSE, id: course:CSE435, name: 'Compiler Design', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:CSE436 CONTENT { course_code: 'CSE436', credit: 1f, department: department:CSE, id: course:CSE436, name: 'Compiler Design Lab', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:CSE437 CONTENT { course_code: 'CSE437', credit: 3f, department: department:CSE, id: course:CSE437, name: 'Computer Graphics', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:CSE438 CONTENT { course_code: 'CSE438', credit: 1f, department: department:CSE, id: course:CSE438, name: 'Computer Graphics Lab', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:CSE439 CONTENT { course_code: 'CSE439', credit: 3f, department: department:CSE, id: course:CSE439, name: 'Data Mining', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:CSE440 CONTENT { course_code: 'CSE440', credit: 1f, department: department:CSE, id: course:CSE440, name: 'Data Mining Lab', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:ECO251 CONTENT { course_code: 'ECO251', credit: 3f, department: department:ECO, id: course:ECO251, name: 'Microeconomics', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:ECO252 CONTENT { course_code: 'ECO252', credit: 3f, department: department:ECO, id: course:ECO252, name: 'Macroeconomics', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:ECO253 CONTENT { course_code: 'ECO253', credit: 3f, department: department:ECO, id: course:ECO253, name: 'International Economics', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:ECO254 CONTENT { course_code: 'ECO254', credit: 3f, department: department:ECO, id: course:ECO254, name: 'Econometrics', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:ECO255 CONTENT { course_code: 'ECO255', credit: 1f, department: department:ECO, id: course:ECO255, name: 'Econometrics Lab', semester: 7, type: 'Lab', year: 2019 };
UPDATE course:EEE341 CONTENT { course_code: 'EEE341', credit: 3f, department: department:EEE, id: course:EEE341, name: 'Power Systems', semester: 4, type: 'Theory', year: 2019 };
UPDATE course:EEE342 CONTENT { course_code: 'EEE342', credit: 1f, department: department:EEE, id: course:EEE342, name: 'Power Systems Lab', semester: 4, type: 'Lab', year: 2019 };
UPDATE course:EEE343 CONTENT { course_code: 'EEE343', credit: 3f, department: department:EEE, id: course:EEE343, name: 'Electromagnetics', semester: 7, type: 'Theory', year: 2019 };
UPDATE course:EEE344 CONTENT { course_code: 'EEE344', credit: 1f, department: department:EEE, id: course:EEE344, name: 'Electromagnetics Lab', semester: 7, type: 'Lab', year: 2019 };

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
-- TABLE DATA: exam
-- ------------------------------

UPDATE exam:rxapyq6ha3clq9andenh CONTENT { creation_date: '2024-04-16', details: 'Final Exam Details', due_date: '2024-04-17', id: exam:rxapyq6ha3clq9andenh, title: 'Final Exam', total_grade: '60' };
UPDATE exam:y94xvbnh3mzv81pt0jco CONTENT { creation_date: '2024-04-16', details: 'exam announcements', due_date: '2024-01-01', id: exam:y94xvbnh3mzv81pt0jco, title: 'New Exam', total_grade: '25' };

-- ------------------------------
-- TABLE DATA: follows
-- ------------------------------

RELATE course:CSE330 -> follows:13wrgnxgn193h41lex3a -> routine:196j2rikt9dgikhpuelv CONTENT { __: true, day: 'Wednesday', id: follows:13wrgnxgn193h41lex3a, in: course:CSE330, out: routine:196j2rikt9dgikhpuelv, year: '2023' };
RELATE course:CSE222 -> follows:4gyn6go2nt1z45jw91bc -> routine:0890gz6xvnjc5oc9cgfc CONTENT { __: true, day: 'Monday', id: follows:4gyn6go2nt1z45jw91bc, in: course:CSE222, out: routine:0890gz6xvnjc5oc9cgfc, year: '2023' };
RELATE course:CSE222 -> follows:5r64y6ulhrhdf8zapyax -> routine:0890gz6xvnjc5oc9cgfc CONTENT { __: true, day: 'Sunday', id: follows:5r64y6ulhrhdf8zapyax, in: course:CSE222, out: routine:0890gz6xvnjc5oc9cgfc, year: '2023' };

-- ------------------------------
-- TABLE DATA: has
-- ------------------------------

RELATE course:CSE222 -> has:5gizd5fhz5e6786v615o -> assignment:yefg4300s6h353zi8e14 CONTENT { __: true, id: has:5gizd5fhz5e6786v615o, in: course:CSE222, out: assignment:yefg4300s6h353zi8e14 };
RELATE course:CSE222 -> has:ee8j9b44rytcua6sq1bb -> exam:y94xvbnh3mzv81pt0jco CONTENT { __: true, id: has:ee8j9b44rytcua6sq1bb, in: course:CSE222, out: exam:y94xvbnh3mzv81pt0jco };
RELATE course:CSE222 -> has:gdwh0a4n59m0wa8wukzk -> resource:a2lphbzcgp5ki4xe5jtv CONTENT { __: true, id: has:gdwh0a4n59m0wa8wukzk, in: course:CSE222, out: resource:a2lphbzcgp5ki4xe5jtv, type: 'Video', year: '2023' };
RELATE course:CSE222 -> has:ivx3x5biqz2xoo003290 -> assignment:zdq3idbfjm9qaa0vrfrg CONTENT { __: true, id: has:ivx3x5biqz2xoo003290, in: course:CSE222, out: assignment:zdq3idbfjm9qaa0vrfrg };
RELATE course:CSE222 -> has:l8vnt4ktsuejz28tu4ec -> exam:rxapyq6ha3clq9andenh CONTENT { __: true, id: has:l8vnt4ktsuejz28tu4ec, in: course:CSE222, out: exam:rxapyq6ha3clq9andenh };
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

RELATE student:2023119144 -> login:0rmjsdqk2j0v0qq2thwv -> user:emfrsjin5aql5nbu2cnz CONTENT { __: true, id: login:0rmjsdqk2j0v0qq2thwv, in: student:2023119144, out: user:emfrsjin5aql5nbu2cnz };
RELATE teacher:2019331073 -> login:1sjkmi7o2a9f98a978gi -> user:ws6uynksgs1kdr07b3r8 CONTENT { __: true, id: login:1sjkmi7o2a9f98a978gi, in: teacher:2019331073, out: user:ws6uynksgs1kdr07b3r8 };
RELATE admin:2022331001 -> login:2j7g07vcx7eznorpz4ue -> user:h5qjh4wllongbemlw42x CONTENT { __: true, id: login:2j7g07vcx7eznorpz4ue, in: admin:2022331001, out: user:h5qjh4wllongbemlw42x };
RELATE student:2019331013 -> login:2or2pea30ymoa6ko5905 -> user:xz5o4prrbak776nivu4a CONTENT { __: true, id: login:2or2pea30ymoa6ko5905, in: student:2019331013, out: user:xz5o4prrbak776nivu4a };
RELATE student:2019331073 -> login:34sbarqyky02ph54s3dg -> user:aasa01kbaknch7ol17w7 CONTENT { __: true, id: login:34sbarqyky02ph54s3dg, in: student:2019331073, out: user:aasa01kbaknch7ol17w7 };
RELATE teacher:2023331009 -> login:39xhoz0lvl1rf4np4vn6 -> user:e5o3mivkkcvdrv1jaazl CONTENT { __: true, id: login:39xhoz0lvl1rf4np4vn6, in: teacher:2023331009, out: user:e5o3mivkkcvdrv1jaazl };
RELATE admin:2026331006 -> login:4fnpnlbzkvp97ec3ovnr -> user:fw16ceg0wvr699m5bi5g CONTENT { __: true, id: login:4fnpnlbzkvp97ec3ovnr, in: admin:2026331006, out: user:fw16ceg0wvr699m5bi5g };
RELATE admin:2029331009 -> login:5cjw39ul5v036gnagjx8 -> user:s87eei68m6ieqggafm04 CONTENT { __: true, id: login:5cjw39ul5v036gnagjx8, in: admin:2029331009, out: user:s87eei68m6ieqggafm04 };
RELATE admin:2028331008 -> login:5kmyonq6ygeky9e3vk68 -> user:acpfio57duapbp19w16f CONTENT { __: true, id: login:5kmyonq6ygeky9e3vk68, in: admin:2028331008, out: user:acpfio57duapbp19w16f };
RELATE student:2022998101 -> login:74aiw92bacx0k2zvodbu -> user:4p3m8nugy0lspbz2mxr8 CONTENT { __: true, id: login:74aiw92bacx0k2zvodbu, in: student:2022998101, out: user:4p3m8nugy0lspbz2mxr8 };
RELATE teacher:2020331006 -> login:85r3fh6pem3ykbo6l46q -> user:dix9jmk14tjw3mr13r8r CONTENT { __: true, id: login:85r3fh6pem3ykbo6l46q, in: teacher:2020331006, out: user:dix9jmk14tjw3mr13r8r };
RELATE student:2022887055 -> login:ad56liuqu1nfkj275cz5 -> user:pi75cobhfzu0xp5ex832 CONTENT { __: true, id: login:ad56liuqu1nfkj275cz5, in: student:2022887055, out: user:pi75cobhfzu0xp5ex832 };
RELATE student:2019331073 -> login:blpeo81xgqwqadat5o9y -> user:ws6uynksgs1kdr07b3r8 CONTENT { __: true, id: login:blpeo81xgqwqadat5o9y, in: student:2019331073, out: user:ws6uynksgs1kdr07b3r8 };
RELATE admin:2031331011 -> login:br8ptlnmxuw6wf6lxqwq -> user:y067zen20ochl6m2mopw CONTENT { __: true, id: login:br8ptlnmxuw6wf6lxqwq, in: admin:2031331011, out: user:y067zen20ochl6m2mopw };
RELATE admin:2024331004 -> login:d420zp4cflazxp3cfmdi -> user:rxwaj49bgjqwkvjzcjz5 CONTENT { __: true, id: login:d420zp4cflazxp3cfmdi, in: admin:2024331004, out: user:rxwaj49bgjqwkvjzcjz5 };
RELATE teacher:2018331005 -> login:dn9dnbgirwmubp1rci24 -> user:d6m4gk2kinumvumj8mp2 CONTENT { __: true, id: login:dn9dnbgirwmubp1rci24, in: teacher:2018331005, out: user:d6m4gk2kinumvumj8mp2 };
RELATE admin:2030331010 -> login:e4v6pj64o1po3a84qhyk -> user:ecfcoo0pvjhel2kww7k0 CONTENT { __: true, id: login:e4v6pj64o1po3a84qhyk, in: admin:2030331010, out: user:ecfcoo0pvjhel2kww7k0 };
RELATE student:2026331007 -> login:e8qpkjqlbhqdjemtp83e -> user:63vn5cttf0wvbsendum5 CONTENT { __: true, id: login:e8qpkjqlbhqdjemtp83e, in: student:2026331007, out: user:63vn5cttf0wvbsendum5 };
RELATE student:2023119144 -> login:fc5fhrxvg2lqqfouq513 -> user:emfrsjin5aql5nbu2cnz CONTENT { __: true, id: login:fc5fhrxvg2lqqfouq513, in: student:2023119144, out: user:emfrsjin5aql5nbu2cnz };
RELATE teacher:2018331004 -> login:hiuw4om5k2rriobsjt7s -> user:2c5i0340e1ufa6runlqa CONTENT { __: true, id: login:hiuw4om5k2rriobsjt7s, in: teacher:2018331004, out: user:2c5i0340e1ufa6runlqa };
RELATE student:2022331003 -> login:hwenvcwvtn4xltauuyci -> user:fayghqmh7rba674ae68q CONTENT { __: true, id: login:hwenvcwvtn4xltauuyci, in: student:2022331003, out: user:fayghqmh7rba674ae68q };
RELATE admin:2027331007 -> login:ijjetsfwods5a50f6oed -> user:e2yh9xar0v5yr3lcuthl CONTENT { __: true, id: login:ijjetsfwods5a50f6oed, in: admin:2027331007, out: user:e2yh9xar0v5yr3lcuthl };
RELATE admin:2023331003 -> login:izzg9ad5is1f77d9hxjg -> user:diz0chynhwclbnjlxnau CONTENT { __: true, id: login:izzg9ad5is1f77d9hxjg, in: admin:2023331003, out: user:diz0chynhwclbnjlxnau };
RELATE student:2022887055 -> login:j4akp91jlue4wp0w8as7 -> user:pi75cobhfzu0xp5ex832 CONTENT { __: true, id: login:j4akp91jlue4wp0w8as7, in: student:2022887055, out: user:pi75cobhfzu0xp5ex832 };
RELATE teacher:2024331010 -> login:jbrxazo1ggkc02ab07l1 -> user:oultft0c1la1eyesl836 CONTENT { __: true, id: login:jbrxazo1ggkc02ab07l1, in: teacher:2024331010, out: user:oultft0c1la1eyesl836 };
RELATE student:2022998101 -> login:je99cbt2l41mku1oniug -> user:4p3m8nugy0lspbz2mxr8 CONTENT { __: true, id: login:je99cbt2l41mku1oniug, in: student:2022998101, out: user:4p3m8nugy0lspbz2mxr8 };
RELATE student:2023331004 -> login:jj09juftwg2ghkj44h49 -> user:wtl0pb9d8fg47m5dzb7u CONTENT { __: true, id: login:jj09juftwg2ghkj44h49, in: student:2023331004, out: user:wtl0pb9d8fg47m5dzb7u };
RELATE teacher:2019331073 -> login:mxfmvd6g8spqrhptat6b -> user:aasa01kbaknch7ol17w7 CONTENT { __: true, id: login:mxfmvd6g8spqrhptat6b, in: teacher:2019331073, out: user:aasa01kbaknch7ol17w7 };
RELATE student:2028331009 -> login:otgwpvk0yhitsbzhmwt3 -> user:tl9wvmebiqz34ccalyao CONTENT { __: true, id: login:otgwpvk0yhitsbzhmwt3, in: student:2028331009, out: user:tl9wvmebiqz34ccalyao };
RELATE teacher:2026331012 -> login:oztuub3vu7pm28yztmfd -> user:ly0cwtsrc9jjcraufey5 CONTENT { __: true, id: login:oztuub3vu7pm28yztmfd, in: teacher:2026331012, out: user:ly0cwtsrc9jjcraufey5 };
RELATE student:2021331002 -> login:p966mx3kfplfhvr1h8bd -> user:qo9qify1bi2fux92hfi3 CONTENT { __: true, id: login:p966mx3kfplfhvr1h8bd, in: student:2021331002, out: user:qo9qify1bi2fux92hfi3 };
RELATE student:2019331013 -> login:pmkrvgbua8wxafmad527 -> user:xz5o4prrbak776nivu4a CONTENT { __: true, id: login:pmkrvgbua8wxafmad527, in: student:2019331013, out: user:xz5o4prrbak776nivu4a };
RELATE student:2022998101 -> login:pupihh1qmjmigs8dcjky -> user:4p3m8nugy0lspbz2mxr8 CONTENT { __: true, id: login:pupihh1qmjmigs8dcjky, in: student:2022998101, out: user:4p3m8nugy0lspbz2mxr8 };
RELATE student:2023119144 -> login:qj5r8skr91sou3r18d1n -> user:emfrsjin5aql5nbu2cnz CONTENT { __: true, id: login:qj5r8skr91sou3r18d1n, in: student:2023119144, out: user:emfrsjin5aql5nbu2cnz };
RELATE admin:2019331004 -> login:s1ut92rd7ukzly37hbvr -> user:hb4sfhwnejl13tkzhfpw CONTENT { __: true, id: login:s1ut92rd7ukzly37hbvr, in: admin:2019331004, out: user:hb4sfhwnejl13tkzhfpw };
RELATE teacher:2017331003 -> login:s4ktmc5yzwlie89gedoz -> user:3bzbitac31n2x5azkogd CONTENT { __: true, id: login:s4ktmc5yzwlie89gedoz, in: teacher:2017331003, out: user:3bzbitac31n2x5azkogd };
RELATE admin:2019331004 -> login:v1n9llzjn1d1dpohwhza -> user:hb4sfhwnejl13tkzhfpw CONTENT { __: true, id: login:v1n9llzjn1d1dpohwhza, in: admin:2019331004, out: user:hb4sfhwnejl13tkzhfpw };
RELATE student:2027331008 -> login:w7bxir8r8vwmrz0hs3va -> user:7gko5yns2t2lm6ld8hb7 CONTENT { __: true, id: login:w7bxir8r8vwmrz0hs3va, in: student:2027331008, out: user:7gko5yns2t2lm6ld8hb7 };
RELATE student:2029331010 -> login:whpdfrylnlh53h66e0x7 -> user:qaqsmecwjcuesyrxz6c0 CONTENT { __: true, id: login:whpdfrylnlh53h66e0x7, in: student:2029331010, out: user:qaqsmecwjcuesyrxz6c0 };
RELATE teacher:2022331008 -> login:wxuf1fzb4juko4wpv4v7 -> user:786385avszz3sji0e0sq CONTENT { __: true, id: login:wxuf1fzb4juko4wpv4v7, in: teacher:2022331008, out: user:786385avszz3sji0e0sq };
RELATE student:2025331006 -> login:xfehupcy11ivsqvcqhym -> user:r04ju7d82ojwwpwbxzzv CONTENT { __: true, id: login:xfehupcy11ivsqvcqhym, in: student:2025331006, out: user:r04ju7d82ojwwpwbxzzv };
RELATE teacher:2019331005 -> login:y7dpfqcgydwee8cwwwkd -> user:g72wh4uz3j2bvo350nqa CONTENT { __: true, id: login:y7dpfqcgydwee8cwwwkd, in: teacher:2019331005, out: user:g72wh4uz3j2bvo350nqa };
RELATE student:2024331005 -> login:yhca7vlwdyfyvd0t9j8m -> user:hn41yodgfd5rad996qui CONTENT { __: true, id: login:yhca7vlwdyfyvd0t9j8m, in: student:2024331005, out: user:hn41yodgfd5rad996qui };
RELATE student:2020331001 -> login:yz1jm6flvp4smztwrxxw -> user:yqjhe8r14hmnobsqgaci CONTENT { __: true, id: login:yz1jm6flvp4smztwrxxw, in: student:2020331001, out: user:yqjhe8r14hmnobsqgaci };
RELATE admin:2025331005 -> login:zyafjnsxdml1l40davei -> user:3x7xfzq2uzita4xydaj2 CONTENT { __: true, id: login:zyafjnsxdml1l40davei, in: admin:2025331005, out: user:3x7xfzq2uzita4xydaj2 };

-- ------------------------------
-- TABLE DATA: notice
-- ------------------------------

UPDATE notice:h180mgl5jb8uve0g7zql CONTENT { creation_date: '4/16/2024', creator: 'Office of the developer', department: [department:ECO, department:EEE, department:PHY, department:SWE], details: 'This notice does not include CSE in recievers', id: notice:h180mgl5jb8uve0g7zql, title: 'Another Notice' };
UPDATE notice:s706b46vny551yqsijrg CONTENT { creation_date: '4/16/2024', creator: 'Office of the developer', department: [department:CSE, department:ECO, department:EEE, department:PHY, department:SWE], details: 'New Title Details', id: notice:s706b46vny551yqsijrg, title: 'New Title' };

-- ------------------------------
-- TABLE DATA: notify
-- ------------------------------


-- ------------------------------
-- TABLE DATA: offers
-- ------------------------------


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
UPDATE student:2020331001 CONTENT { blood_group: 'A+', current_semester: 8, department: department:CSE, email: { academic: 'student1@student.sust.edu', personal: 'student1@example.com' }, gender: 'male', id: student:2020331001, name: 'John Smith', personal: { birthday: '2000-01-15T00:00:00Z', father: 'Robert Smith', hometown: 'Dhaka', mother: 'Emily Smith', phone: '01711112233' }, privilage: 'Default', session: 2020 };
UPDATE student:2021331002 CONTENT { blood_group: 'B+', current_semester: 6, department: department:EEE, email: { academic: 'student2@student.sust.edu', personal: 'student2@example.com' }, gender: 'female', id: student:2021331002, name: 'Sarah Johnson', personal: { birthday: '2001-03-20T00:00:00Z', father: 'Michael Johnson', hometown: 'Chittagong', mother: 'Karen Johnson', phone: '01711223344' }, privilage: 'Default', session: 2021 };
UPDATE student:2022331003 CONTENT { blood_group: 'AB+', current_semester: 4, department: department:ECO, email: { academic: 'student3@student.sust.edu', personal: 'student3@example.com' }, gender: 'male', id: student:2022331003, name: 'David Lee', personal: { birthday: '2002-07-10T00:00:00Z', father: 'Robert Lee', hometown: 'Sylhet', mother: 'Jessica Lee', phone: '01711334455' }, privilage: 'Default', session: 2022 };
UPDATE student:2022887055 CONTENT { blood_group: 'A-', current_semester: 2, department: department:ECO, email: { academic: 'sophia03@student.sust.edu', personal: 'sophia.ahmed@example.com' }, gender: 'female', id: student:2022887055, name: 'Sophia Ahmed', personal: { birthday: '2001-09-10T14:00:00Z', father: 'Kamal Ahmed', hometown: 'Chittagong', mother: 'Sara Ahmed', phone: '1754321098' }, session: 2022 };
UPDATE student:2022998101 CONTENT { blood_group: 'B-', current_semester: 4, department: department:SWE, email: { academic: 'arjun04@student.sust.edu', personal: 'arjun.chowdhury@example.com' }, gender: 'male', id: student:2022998101, name: 'Arjun Chowdhury', personal: { birthday: '2001-07-12T09:15:00Z', father: 'Rahim Chowdhury', hometown: 'Rajshahi', mother: 'Nasreen Chowdhury', phone: '1743210989' }, session: 2022 };
UPDATE student:2023119144 CONTENT { blood_group: 'O+', current_semester: 5, department: department:PHY, email: { academic: 'amina05@student.sust.edu', personal: 'amina.islam@example.com' }, gender: 'female', id: student:2023119144, name: 'Amina Islam', personal: { birthday: '2001-10-08T10:00:00Z', father: 'Mohammed Islam', hometown: 'Khulna', mother: 'Fatima Islam', phone: '1732109877' }, session: 2022 };
UPDATE student:2023331004 CONTENT { blood_group: 'O+', current_semester: 2, department: department:PHY, email: { academic: 'student4@student.sust.edu', personal: 'student4@example.com' }, gender: 'female', id: student:2023331004, name: 'Emily Wilson', personal: { birthday: '2003-11-25T00:00:00Z', father: 'Michael Wilson', hometown: 'Khulna', mother: 'Sarah Wilson', phone: '01711445566' }, privilage: 'Default', session: 2023 };
UPDATE student:2024331005 CONTENT { blood_group: 'A-', current_semester: 8, department: department:CSE, email: { academic: 'student5@student.sust.edu', personal: 'student5@example.com' }, gender: 'male', id: student:2024331005, name: 'William Brown', personal: { birthday: '2000-04-01T00:00:00Z', father: 'Robert Brown', hometown: 'Rajshahi', mother: 'Elizabeth Brown', phone: '01711556677' }, privilage: 'Default', session: 2024 };
UPDATE student:2025331006 CONTENT { blood_group: 'B-', current_semester: 6, department: department:EEE, email: { academic: 'student6@student.sust.edu', personal: 'student6@example.com' }, gender: 'female', id: student:2025331006, name: 'Sophia Davis', personal: { birthday: '2001-08-14T00:00:00Z', father: 'Michael Davis', hometown: 'Barishal', mother: 'Jessica Davis', phone: '01711667788' }, privilage: 'Default', session: 2019 };
UPDATE student:2026331007 CONTENT { blood_group: 'AB-', current_semester: 4, department: department:ECO, email: { academic: 'student7@student.sust.edu', personal: 'student7@example.com' }, gender: 'male', id: student:2026331007, name: 'Michael Thompson', personal: { birthday: '2002-12-05T00:00:00Z', father: 'Robert Thompson', hometown: 'Rangpur', mother: 'Karen Thompson', phone: '01711778899' }, privilage: 'Default', session: 2019 };
UPDATE student:2027331008 CONTENT { blood_group: 'O-', current_semester: 2, department: department:PHY, email: { academic: 'student8@student.sust.edu', personal: 'student8@example.com' }, gender: 'female', id: student:2027331008, name: 'Emily Garcia', personal: { birthday: '2003-06-18T00:00:00Z', father: 'Michael Garcia', hometown: 'Cumilla', mother: 'Jessica Garcia', phone: '01711889900' }, privilage: 'Default', session: 2019 };
UPDATE student:2028331009 CONTENT { blood_group: 'A+', current_semester: 8, department: department:CSE, email: { academic: 'student9@student.sust.edu', personal: 'student9@example.com' }, gender: 'male', id: student:2028331009, name: 'David Martinez', personal: { birthday: '2000-09-30T00:00:00Z', father: 'Robert Martinez', hometown: 'Feni', mother: 'Elizabeth Martinez', phone: '01711990011' }, privilage: 'Default', session: 2019 };
UPDATE student:2029331010 CONTENT { blood_group: 'B+', current_semester: 6, department: department:EEE, email: { academic: 'student10@student.sust.edu', personal: 'student10@example.com' }, gender: 'female', id: student:2029331010, name: 'Sarah Rodriguez', personal: { birthday: '2001-04-22T00:00:00Z', father: 'Michael Rodriguez', hometown: 'Noakhali', mother: 'Karen Rodriguez', phone: '01711001122' }, privilage: 'Default', session: 2019 };

-- ------------------------------
-- TABLE DATA: taken
-- ------------------------------

RELATE class:j58fg1x9gbo1gm8ye16w -> taken:14r5zix1ea3q7syn0xar -> course:CSE222 CONTENT { __: true, id: taken:14r5zix1ea3q7syn0xar, in: class:j58fg1x9gbo1gm8ye16w, out: course:CSE222 };
RELATE class:sy4depxxixiu1nuh0cn5 -> taken:68ksdxyw4ijpa5nbs5qu -> course:CSE222 CONTENT { __: true, id: taken:68ksdxyw4ijpa5nbs5qu, in: class:sy4depxxixiu1nuh0cn5, out: course:CSE222 };
RELATE class:plc2xz2k1zf0jj0nx2oy -> taken:91rnx402awzve1394fbn -> course:CSE222 CONTENT { __: true, id: taken:91rnx402awzve1394fbn, in: class:plc2xz2k1zf0jj0nx2oy, out: course:CSE222 };
RELATE class:iko3h1d93ue6ykr5648i -> taken:9gtbv9fgen29hifb04ab -> course:CSE330 CONTENT { __: true, id: taken:9gtbv9fgen29hifb04ab, in: class:iko3h1d93ue6ykr5648i, out: course:CSE330 };
RELATE class:5u3rkbkntqobqetfrite -> taken:9q0m4z01aml1xzgzlct1 -> course:CSE222 CONTENT { __: true, id: taken:9q0m4z01aml1xzgzlct1, in: class:5u3rkbkntqobqetfrite, out: course:CSE222 };
RELATE class:xvnxfywsx06cwiohay7z -> taken:qgqd03x7j7p15ncrhu3l -> course:CSE222 CONTENT { __: true, id: taken:qgqd03x7j7p15ncrhu3l, in: class:xvnxfywsx06cwiohay7z, out: course:CSE222 };

-- ------------------------------
-- TABLE DATA: takes
-- ------------------------------

RELATE student:2019331013 -> takes:4tnahxcwnlgpj6l634dl -> course:CSE222 CONTENT { __: true, id: takes:4tnahxcwnlgpj6l634dl, in: student:2019331013, out: course:CSE222, semester: 12, year: 2019 };
RELATE student:2020331001 -> takes:58f6fq9wa5swrkqti3jf -> course:CSE432 CONTENT { __: true, id: takes:58f6fq9wa5swrkqti3jf, in: student:2020331001, out: course:CSE432 };
RELATE student:2019331073 -> takes:8783thlijgk4el3q7b4y -> course:CSE222 CONTENT { __: true, id: takes:8783thlijgk4el3q7b4y, in: student:2019331073, out: course:CSE222 };
RELATE student:2020331001 -> takes:bhk9twkxgsioenfpnnhd -> course:CSE330 CONTENT { __: true, id: takes:bhk9twkxgsioenfpnnhd, in: student:2020331001, out: course:CSE330 };
RELATE student:2019331013 -> takes:cvq6uvnqf51c3vvuws97 -> course:CSE330 CONTENT { __: true, id: takes:cvq6uvnqf51c3vvuws97, in: student:2019331013, out: course:CSE330, semester: 12, year: 2019 };
RELATE student:2020331001 -> takes:um3ehoqs4etcw7p3luqq -> course:CSE222 CONTENT { __: true, id: takes:um3ehoqs4etcw7p3luqq, in: student:2020331001, out: course:CSE222 };

-- ------------------------------
-- TABLE DATA: teacher
-- ------------------------------

UPDATE teacher:2017331003 CONTENT { blood_group: 'AB+', department: department:EEE, designation: 'Assistant Professor', email: { academic: 'teacher3@sust.edu', personal: 'teacher3@example.com' }, gender: 'male', id: teacher:2017331003, name: 'David Lee', personal: { birthday: '1985-03-10T00:00:00Z', father: 'Robert Lee', hometown: 'Sylhet', mother: 'Jessica Lee', phone: '01711334455' } };
UPDATE teacher:2018331004 CONTENT { blood_group: 'O+', department: department:ECO, designation: 'Lecturer', email: { academic: 'teacher4@sust.edu', personal: 'teacher4@example.com' }, gender: 'female', id: teacher:2018331004, name: 'Emily Wilson', personal: { birthday: '1990-11-25T00:00:00Z', father: 'Michael Wilson', hometown: 'Khulna', mother: 'Sarah Wilson', phone: '01711445566' } };
UPDATE teacher:2018331005 CONTENT { blood_group: 'B+', department: department:CSE, designation: 'Assistant Professor', email: { academic: 'teacher1@sust.edu', personal: 'teacher1@example.com' }, gender: 'male', id: teacher:2018331005, name: 'David Wilson', personal: { birthday: '1990-11-03T00:00:00Z', father: 'Robert Wilson', hometown: 'Sylhet', mother: 'Emily Wilson', phone: '01711665544' } };
UPDATE teacher:2019331005 CONTENT { blood_group: 'A-', department: department:PHY, designation: 'Professor', email: { academic: 'teacher5@sust.edu', personal: 'teacher5@example.com' }, gender: 'male', id: teacher:2019331005, name: 'William Brown', personal: { birthday: '1970-07-01T00:00:00Z', father: 'Robert Brown', hometown: 'Rajshahi', mother: 'Elizabeth Brown', phone: '01711556677' } };
UPDATE teacher:2019331073 CONTENT { blood_group: 'B+', department: department:CSE, designation: 'Lecturer', email: { academic: 'mufassir73@sust.edu', personal: 'mac22214u@gmail.com' }, gender: 'male', id: teacher:2019331073, name: 'Mufassir Ahmad Chowdhury', personal: { birthday: '2001-07-10T07:18:52Z', father: 'Hafiz Md Mashhud Chowdhury', hometown: 'Sylhet', mother: 'Afsana Begum', phone: '1771144308' } };
UPDATE teacher:2019331088 CONTENT { blood_group: 'A+', department: department:CSE, designation: 'Lecturer', email: { academic: 'sajid88@sust.edu', personal: 'sajid@gmail.com' }, gender: 'male', id: teacher:2019331088, name: 'Sajid Zakaria', personal: { birthday: '2000-12-31T18:00:00Z', father: 'Sajid', hometown: 'Sylhet', mother: 'Sajida', phone: '01771144388' } };
UPDATE teacher:2020331006 CONTENT { blood_group: 'B-', department: department:CSE, designation: 'Assistant Professor', email: { academic: 'teacher6@sust.edu', personal: 'teacher6@example.com' }, gender: 'female', id: teacher:2020331006, name: 'Sophia Davis', personal: { birthday: '1988-02-14T00:00:00Z', father: 'Michael Davis', hometown: 'Barishal', mother: 'Jessica Davis', phone: '01711667788' } };
UPDATE teacher:2022331008 CONTENT { blood_group: 'O-', department: department:ECO, designation: 'Lecturer', email: { academic: 'teacher8@sust.edu', personal: 'teacher8@example.com' }, gender: 'female', id: teacher:2022331008, name: 'Emily Garcia', personal: { birthday: '1993-06-18T00:00:00Z', father: 'Michael Garcia', hometown: 'Cumilla', mother: 'Jessica Garcia', phone: '01711889900' } };
UPDATE teacher:2023331009 CONTENT { blood_group: 'A+', department: department:PHY, designation: 'Professor', email: { academic: 'teacher9@sust.edu', personal: 'teacher9@example.com' }, gender: 'male', id: teacher:2023331009, name: 'David Martinez', personal: { birthday: '1972-09-30T00:00:00Z', father: 'Robert Martinez', hometown: 'Feni', mother: 'Elizabeth Martinez', phone: '01711990011' } };
UPDATE teacher:2024331010 CONTENT { blood_group: 'B+', department: department:CSE, designation: 'Assistant Professor', email: { academic: 'teacher10@sust.edu', personal: 'teacher10@example.com' }, gender: 'female', id: teacher:2024331010, name: 'Sarah Rodriguez', personal: { birthday: '1986-04-22T00:00:00Z', father: 'Michael Rodriguez', hometown: 'Noakhali', mother: 'Karen Rodriguez', phone: '01711001122' } };
UPDATE teacher:2026331012 CONTENT { blood_group: 'O+', department: department:ECO, designation: 'Lecturer', email: { academic: 'teacher12@sust.edu', personal: 'teacher12@example.com' }, gender: 'female', id: teacher:2026331012, name: 'Emily Gonzalez', personal: { birthday: '1992-08-05T00:00:00Z', father: 'Michael Gonzalez', hometown: 'Brahmanbaria', mother: 'Sarah Gonzalez', phone: '01711223344' } };

-- ------------------------------
-- TABLE DATA: teaches
-- ------------------------------

RELATE teacher:2022331008 -> teaches:0v7a5zv3dybf0dtpw69x -> course:ECO252 CONTENT { __: true, id: teaches:0v7a5zv3dybf0dtpw69x, in: teacher:2022331008, out: course:ECO252 };
RELATE teacher:2017331003 -> teaches:0vou1lddl8xpgckom2ew -> course:CSE222 CONTENT { __: true, id: teaches:0vou1lddl8xpgckom2ew, in: teacher:2017331003, out: course:CSE222 };
RELATE teacher:2018331005 -> teaches:1lw5u2ene1vmmyp0noj6 -> course:CSE222 CONTENT { __: true, id: teaches:1lw5u2ene1vmmyp0noj6, in: teacher:2018331005, out: course:CSE222 };
RELATE teacher:2019331073 -> teaches:1t8a2k9lz8q7c1mc4p6u -> course:CSE222 CONTENT { __: true, id: teaches:1t8a2k9lz8q7c1mc4p6u, in: teacher:2019331073, out: course:CSE222 };
RELATE teacher:2019331073 -> teaches:3dhcx2nkh6ocyz5pkr5w -> course:CSE330 CONTENT { __: true, id: teaches:3dhcx2nkh6ocyz5pkr5w, in: teacher:2019331073, out: course:CSE330 };
RELATE teacher:2018331005 -> teaches:6hxr3zsw37m5a1tz0lc8 -> course:CSE330 CONTENT { __: true, id: teaches:6hxr3zsw37m5a1tz0lc8, in: teacher:2018331005, out: course:CSE330 };
RELATE teacher:2018331005 -> teaches:6v0a9l3dw8x3r4ipc38r -> course:CSE333 CONTENT { __: true, id: teaches:6v0a9l3dw8x3r4ipc38r, in: teacher:2018331005, out: course:CSE333 };
RELATE teacher:2018331004 -> teaches:8uwss8gmd8ik9wkdcark -> course:CSE222 CONTENT { __: true, id: teaches:8uwss8gmd8ik9wkdcark, in: teacher:2018331004, out: course:CSE222 };
RELATE teacher:2019331005 -> teaches:b29499oza8hdiq2hfnkz -> course:ECO251 CONTENT { __: true, id: teaches:b29499oza8hdiq2hfnkz, in: teacher:2019331005, out: course:ECO251 };
RELATE teacher:2019331005 -> teaches:b5cn6z7e2xwg8wy9um11 -> course:ECO253 CONTENT { __: true, id: teaches:b5cn6z7e2xwg8wy9um11, in: teacher:2019331005, out: course:ECO253 };
RELATE teacher:2017331003 -> teaches:b7qxwmykcm2to0jz7i7a -> course:EEE341 CONTENT { __: true, id: teaches:b7qxwmykcm2to0jz7i7a, in: teacher:2017331003, out: course:EEE341 };
RELATE teacher:2019331005 -> teaches:c0jco9nl1lkydev0z45v -> course:ECO254 CONTENT { __: true, id: teaches:c0jco9nl1lkydev0z45v, in: teacher:2019331005, out: course:ECO254 };
RELATE teacher:2019331005 -> teaches:cty2rh3gfzu9rqfaplus -> course:ECO255 CONTENT { __: true, id: teaches:cty2rh3gfzu9rqfaplus, in: teacher:2019331005, out: course:ECO255 };
RELATE teacher:2017331003 -> teaches:e4b59334gfepxfvojvzy -> course:EEE343 CONTENT { __: true, id: teaches:e4b59334gfepxfvojvzy, in: teacher:2017331003, out: course:EEE343 };
RELATE teacher:2017331003 -> teaches:f3j8ghoe3i7zxrxjb35e -> course:EEE344 CONTENT { __: true, id: teaches:f3j8ghoe3i7zxrxjb35e, in: teacher:2017331003, out: course:EEE344 };
RELATE teacher:2019331009 -> teaches:g2b8jq5pn8w11l98nh49 -> course:PHY253 CONTENT { __: true, id: teaches:g2b8jq5pn8w11l98nh49, in: teacher:2019331009, out: course:PHY253 };
RELATE teacher:2019331009 -> teaches:h1l7ovyre4egbpxzaisi -> course:PHY254 CONTENT { __: true, id: teaches:h1l7ovyre4egbpxzaisi, in: teacher:2019331009, out: course:PHY254 };
RELATE teacher:2019331009 -> teaches:hn8i7h33hc7ynx9nh9wm -> course:PHY251 CONTENT { __: true, id: teaches:hn8i7h33hc7ynx9nh9wm, in: teacher:2019331009, out: course:PHY251 };
RELATE teacher:2019331009 -> teaches:i5m3vpnogvjzi9u4wyla -> course:PHY255 CONTENT { __: true, id: teaches:i5m3vpnogvjzi9u4wyla, in: teacher:2019331009, out: course:PHY255 };
RELATE teacher:2020331006 -> teaches:j2q4ckl1jsjiletcgqmf -> course:CSE435 CONTENT { __: true, id: teaches:j2q4ckl1jsjiletcgqmf, in: teacher:2020331006, out: course:CSE435 };
RELATE teacher:2020331006 -> teaches:jf05462za7r10ge1pocp -> course:CSE222 CONTENT { __: true, id: teaches:jf05462za7r10ge1pocp, in: teacher:2020331006, out: course:CSE222 };
RELATE teacher:2020331006 -> teaches:k8p6w7gu3dv43bv2h6b3 -> course:CSE436 CONTENT { __: true, id: teaches:k8p6w7gu3dv43bv2h6b3, in: teacher:2020331006, out: course:CSE436 };
RELATE teacher:2020331006 -> teaches:l4f9mvrz5pia7gzzmfjy -> course:CSE437 CONTENT { __: true, id: teaches:l4f9mvrz5pia7gzzmfjy, in: teacher:2020331006, out: course:CSE437 };
RELATE teacher:2019331073 -> teaches:m7e03je0qtchcoiy0n1b -> course:CSE350 CONTENT { __: true, id: teaches:m7e03je0qtchcoiy0n1b, in: teacher:2019331073, out: course:CSE350 };
RELATE teacher:2020331006 -> teaches:m7u2b9djlzkgcyxp9rvq -> course:CSE438 CONTENT { __: true, id: teaches:m7u2b9djlzkgcyxp9rvq, in: teacher:2020331006, out: course:CSE438 };
RELATE teacher:2020331006 -> teaches:n3b7s6wnv8mxl9p1e0kk -> course:CSE439 CONTENT { __: true, id: teaches:n3b7s6wnv8mxl9p1e0kk, in: teacher:2020331006, out: course:CSE439 };
RELATE teacher:2019331009 -> teaches:n3btkzd8yy9dpam4a6j5 -> course:PHY252 CONTENT { __: true, id: teaches:n3btkzd8yy9dpam4a6j5, in: teacher:2019331009, out: course:PHY252 };
RELATE teacher:2020331006 -> teaches:p1t3g4qd0ej8s8ziqdxu -> course:CSE440 CONTENT { __: true, id: teaches:p1t3g4qd0ej8s8ziqdxu, in: teacher:2020331006, out: course:CSE440 };
RELATE teacher:2017331003 -> teaches:r7k29uqwvigk5r70pc3q -> course:EEE342 CONTENT { __: true, id: teaches:r7k29uqwvigk5r70pc3q, in: teacher:2017331003, out: course:EEE342 };

-- ------------------------------
-- TABLE DATA: user
-- ------------------------------

UPDATE user:2c5i0340e1ufa6runlqa CONTENT { email: 'teacher4@sust.edu', id: user:2c5i0340e1ufa6runlqa, password: '$argon2id$v=19$m=19456,t=2,p=1$caNQIqrWoP7ZxHOVkxUeiw$7WRMsvP5jBQj+DZ9pZ6zHXFSG1JWnleyuz3SgKyqJ7I' };
UPDATE user:3bzbitac31n2x5azkogd CONTENT { email: 'teacher3@sust.edu', id: user:3bzbitac31n2x5azkogd, password: '$argon2id$v=19$m=19456,t=2,p=1$ZodfmC+hMu38lnAjjciOiw$p0cYKS225IMKeUCI7jUmGO6avGVBzAV444O28dOHILo' };
UPDATE user:3sx7fe9v6w58gag2b625 CONTENT { email: 'xyz@sust.edu', id: user:3sx7fe9v6w58gag2b625, password: '$argon2id$v=19$m=19456,t=2,p=1$7JLreAVwXh73ALGEEjjtXw$DwpTTl/o2TwlZoooQ+9AC8MqwFCmpiIeGt7T54HU3Jc' };
UPDATE user:3x7xfzq2uzita4xydaj2 CONTENT { email: 'admin5@sust.edu', id: user:3x7xfzq2uzita4xydaj2, password: '$argon2id$v=19$m=19456,t=2,p=1$hIJ+VlQ2P0tf1hKs/m90/A$gjYVZcOaYguDuGVN7P8kGz7lT0IPSHavMkHzhQhdNH8' };
UPDATE user:4p3m8nugy0lspbz2mxr8 CONTENT { email: 'arjun04@student.sust.edu', id: user:4p3m8nugy0lspbz2mxr8, password: '$argon2id$v=19$m=19456,t=2,p=1$W1PLzntnQnQvHVBESwaonw$mqCDsNwfPF/A7Z0WjbkOUmObpzj6kb+QWQyAtoPi/cI' };
UPDATE user:63vn5cttf0wvbsendum5 CONTENT { email: 'student7@student.sust.edu', id: user:63vn5cttf0wvbsendum5, password: '$argon2id$v=19$m=19456,t=2,p=1$aWl9B292fJAA+vjxtkyQkg$truMjLxpBmPk8LT8RfimsFaRmbZ3eg4i+INklqXq5W4' };
UPDATE user:786385avszz3sji0e0sq CONTENT { email: 'teacher8@sust.edu', id: user:786385avszz3sji0e0sq, password: '$argon2id$v=19$m=19456,t=2,p=1$AGrVzkjirba/paMEENSz4A$1kCeizJGKhJsm8IWIDnPs/BAt5iGUP+ijZIWQ6Sm/A4' };
UPDATE user:7gko5yns2t2lm6ld8hb7 CONTENT { email: 'student8@student.sust.edu', id: user:7gko5yns2t2lm6ld8hb7, password: '$argon2id$v=19$m=19456,t=2,p=1$OSClfh5D5UroQRvA0l5IcA$QTqiDKpwwKYDRNh2krKTXvsWKaqk8uSHWX/Yvahvoaw' };
UPDATE user:aasa01kbaknch7ol17w7 CONTENT { email: 'mufassir73@student.sust.edu', id: user:aasa01kbaknch7ol17w7, password: '$argon2id$v=19$m=19456,t=2,p=1$c1fIG/sv0IV5k1UFScZVZg$92aoHf8Y/2wUWTBccYvJ1OmtbwkIAwCkaCksS3+w/b8' };
UPDATE user:acpfio57duapbp19w16f CONTENT { email: 'admin8@sust.edu', id: user:acpfio57duapbp19w16f, password: '$argon2id$v=19$m=19456,t=2,p=1$a846pLfTVR4GAHP0QCP8fw$ZnQRA8XCfMcLZ0+OfqEOuhHhSe52j4CQ7aw6JZOgAsU' };
UPDATE user:d6m4gk2kinumvumj8mp2 CONTENT { email: 'teacher1@sust.edu', id: user:d6m4gk2kinumvumj8mp2, password: '$argon2id$v=19$m=19456,t=2,p=1$+qcChF1l4vzXtow2fSwyAA$dM8MD+IkSC9eRb7XrROqWiw0qTwTee6S/IvRMLc58b0' };
UPDATE user:d9pu7wrq9qgg9ssojw78 CONTENT { email: 'mufassir@sust.edu', id: user:d9pu7wrq9qgg9ssojw78, password: '$argon2id$v=19$m=19456,t=2,p=1$z9bb3s0hsj/Y4eFgiOYeDw$FvNIPOqc39RLOnm1tf3/oALeopjHBh5u8+kDi/BEuWI' };
UPDATE user:dix9jmk14tjw3mr13r8r CONTENT { email: 'teacher6@sust.edu', id: user:dix9jmk14tjw3mr13r8r, password: '$argon2id$v=19$m=19456,t=2,p=1$UFj065EudgEqetJ5kz0PVQ$xu14vJNJokt/gPv3nDKdu07sFZy2OZs8puvvSneZQvU' };
UPDATE user:diz0chynhwclbnjlxnau CONTENT { email: 'admin3@sust.edu', id: user:diz0chynhwclbnjlxnau, password: '$argon2id$v=19$m=19456,t=2,p=1$olW38MDUflglC8ty4vQhZg$6fBhamPikDKhXKRxryuK2EGu/d5aL6oU17nC7SNKnko' };
UPDATE user:e2yh9xar0v5yr3lcuthl CONTENT { email: 'admin7@sust.edu', id: user:e2yh9xar0v5yr3lcuthl, password: '$argon2id$v=19$m=19456,t=2,p=1$DSH0yalnd1TX3pVb/dAcPQ$aeTUlGHrquB79qb306wUuNV/rYcWnjhpYWRssPLAuZI' };
UPDATE user:e5o3mivkkcvdrv1jaazl CONTENT { email: 'teacher9@sust.edu', id: user:e5o3mivkkcvdrv1jaazl, password: '$argon2id$v=19$m=19456,t=2,p=1$otVglKfDOxF/5mtYy86MGA$ue+shNq0sMRfqjXILRYdkWy3mO165mKtvk7PgGsOOEc' };
UPDATE user:ecfcoo0pvjhel2kww7k0 CONTENT { email: 'admin10@sust.edu', id: user:ecfcoo0pvjhel2kww7k0, password: '$argon2id$v=19$m=19456,t=2,p=1$swxekl3uYTpy2FJvL68/aQ$/I6TUwLitvhnNMGaToOYyBpKtpBHCMdmhnsEiOk/dwc' };
UPDATE user:emfrsjin5aql5nbu2cnz CONTENT { email: 'amina05@student.sust.edu', id: user:emfrsjin5aql5nbu2cnz, password: '$argon2id$v=19$m=19456,t=2,p=1$YCAS684IJvTsmkv1vH2CjQ$orcHFgHMZnXhtA8mSKGAGZGEqN4vIN+ynAvvSYFtKWA' };
UPDATE user:fayghqmh7rba674ae68q CONTENT { email: 'student3@student.sust.edu', id: user:fayghqmh7rba674ae68q, password: '$argon2id$v=19$m=19456,t=2,p=1$pWRemoDGLn+Nxh6Ox2FuEg$eYmcv99VP6HNLImI916zLHU/Q1ZunwagVnNbqHX6Zy8' };
UPDATE user:fw16ceg0wvr699m5bi5g CONTENT { email: 'admin6@sust.edu', id: user:fw16ceg0wvr699m5bi5g, password: '$argon2id$v=19$m=19456,t=2,p=1$YpVdCAlP2eXaRVfENHbNaw$39os7dP9Ll8qtjv7c97B4+qQQaVWU6XweZ0EwYQME3g' };
UPDATE user:g72wh4uz3j2bvo350nqa CONTENT { email: 'teacher5@sust.edu', id: user:g72wh4uz3j2bvo350nqa, password: '$argon2id$v=19$m=19456,t=2,p=1$le0XWrDVTuowAfk5tf6bDw$Dy8+MQ4WMZHTyvNOJX5/oKJqWG2vvkNcK3mllmu2Ogg' };
UPDATE user:h5qjh4wllongbemlw42x CONTENT { email: 'admin1@sust.edu', id: user:h5qjh4wllongbemlw42x, password: '$argon2id$v=19$m=19456,t=2,p=1$F+U52uCieDyqIhPZBNVbvQ$lcUQKPyHAJeXNSfJogweiQC6wCPX7VehvuEwahTU9fs' };
UPDATE user:hb4sfhwnejl13tkzhfpw CONTENT { email: 'mrx@sust.edu', id: user:hb4sfhwnejl13tkzhfpw, password: '$argon2id$v=19$m=19456,t=2,p=1$mzO2HyrwcGWzE9TxXkIwHA$UM3Ql1K5ekDY0/yVOC+eLMooNt+U1Oi/ULyI3jTVV8k' };
UPDATE user:hn41yodgfd5rad996qui CONTENT { email: 'student5@student.sust.edu', id: user:hn41yodgfd5rad996qui, password: '$argon2id$v=19$m=19456,t=2,p=1$GczKs2owxEKik9iEfsc3GA$zrflySRqLPdCWQ735kRguY9s+xFXwgMsJ6SrL7rcryQ' };
UPDATE user:ly0cwtsrc9jjcraufey5 CONTENT { email: 'teacher12@sust.edu', id: user:ly0cwtsrc9jjcraufey5, password: '$argon2id$v=19$m=19456,t=2,p=1$EMhuoxeRjSwKlAABa5zgAQ$W3e8WbjPcg7XfWXdFz1DbiXcI2sQQ+m7LITV5HfxiX4' };
UPDATE user:nbe5cbdnq5fke193hw67 CONTENT { email: 'ethan02@student.sust.edu', id: user:nbe5cbdnq5fke193hw67, password: '$argon2id$v=19$m=19456,t=2,p=1$HkXP0cU6hzquhFhok8pdjg$4zkdcs9U/P36y14VFuIjBcHHZI/RVm/gvOi+J3GcdFQ' };
UPDATE user:oultft0c1la1eyesl836 CONTENT { email: 'teacher10@sust.edu', id: user:oultft0c1la1eyesl836, password: '$argon2id$v=19$m=19456,t=2,p=1$RkkWwuD3rkV07Kg1XzmE+w$LmLNGcxmwAGYTj874Y6tRpfR2FuAshMRdIC4rvsXGro' };
UPDATE user:pi75cobhfzu0xp5ex832 CONTENT { email: 'sophia03@student.sust.edu', id: user:pi75cobhfzu0xp5ex832, password: '$argon2id$v=19$m=19456,t=2,p=1$Vm/FqMc7u6x/PuctKb3Qmw$OaOC6GaD4LZu7+5x6163OOJ9jxNEo4y6DSSphcwioew' };
UPDATE user:qaqsmecwjcuesyrxz6c0 CONTENT { email: 'student10@student.sust.edu', id: user:qaqsmecwjcuesyrxz6c0, password: '$argon2id$v=19$m=19456,t=2,p=1$haIcuWo5tgz2Rfo1brT/lQ$Ig0L0n+rTTmkdJdeEp3dCt75GjaQrJ11aQuKJ7f3c9M' };
UPDATE user:qo9qify1bi2fux92hfi3 CONTENT { email: 'student2@student.sust.edu', id: user:qo9qify1bi2fux92hfi3, password: '$argon2id$v=19$m=19456,t=2,p=1$E3UCzeZBCBHmx3gRZdAw3Q$jtjWemYrH5pU+4yd0xCwbRDetYOr6N8WLWiAh3EyDxA' };
UPDATE user:r04ju7d82ojwwpwbxzzv CONTENT { email: 'student6@student.sust.edu', id: user:r04ju7d82ojwwpwbxzzv, password: '$argon2id$v=19$m=19456,t=2,p=1$o9x7Q3SDTrpCJ+i0e2GIuw$Pot/OJkLg6gQRvJE+3a5sGxGVwDgQU117CL9bzioqPs' };
UPDATE user:rxwaj49bgjqwkvjzcjz5 CONTENT { email: 'admin4@sust.edu', id: user:rxwaj49bgjqwkvjzcjz5, password: '$argon2id$v=19$m=19456,t=2,p=1$lR/pWeL3GrK6oA/sP/vS6Q$0bYZMwZr5GnwVgWjpGtTeKVIeMq4d/vMKqQwLzUnI1M' };
UPDATE user:s87eei68m6ieqggafm04 CONTENT { email: 'admin9@sust.edu', id: user:s87eei68m6ieqggafm04, password: '$argon2id$v=19$m=19456,t=2,p=1$+DuZOmDjTdUza189H4Ce3g$5UV+E7JzsGCo9aO5cHYpV8M458Wur51fbym2kGxY3I0' };
UPDATE user:tl9wvmebiqz34ccalyao CONTENT { email: 'student9@student.sust.edu', id: user:tl9wvmebiqz34ccalyao, password: '$argon2id$v=19$m=19456,t=2,p=1$VhsRLtSVlEKMhW4n3H4WBA$4If59Lu/KL30Dc4V8/zchtWzAW3dooqeDDkaI1RPLr0' };
UPDATE user:ws6uynksgs1kdr07b3r8 CONTENT { email: 'mufassir73@sust.edu', id: user:ws6uynksgs1kdr07b3r8, password: '$argon2id$v=19$m=19456,t=2,p=1$mPBgfJGt3zXLfBgw7m4+4A$sG5wE599w7moglTaMHc5ERT8nJNg77mNhB61M91R1Qg' };
UPDATE user:wtl0pb9d8fg47m5dzb7u CONTENT { email: 'student4@student.sust.edu', id: user:wtl0pb9d8fg47m5dzb7u, password: '$argon2id$v=19$m=19456,t=2,p=1$NzhBePzA7bwG2hs+e+5KGw$35xVUNN2nFAYrsyabc0dHXSZYJAaLGnP1zEwLhhQHqQ' };
UPDATE user:xz5o4prrbak776nivu4a CONTENT { email: 'asanul13@student.sust.edu', id: user:xz5o4prrbak776nivu4a, password: '$argon2id$v=19$m=19456,t=2,p=1$bA8wPGQ2nVcvf29yRNdpVg$YlyMWq/crbG+rvpGk4IomFzYf2vnyDs0EtwFWlDXenQ' };
UPDATE user:y067zen20ochl6m2mopw CONTENT { email: 'admin11@sust.edu', id: user:y067zen20ochl6m2mopw, password: '$argon2id$v=19$m=19456,t=2,p=1$gljrmXYpBDDbOnA+gmw7NQ$2dOCOyqBU+EAhp+1JHbh2pI9Os/QaSFSXKVPpmNbzds' };
UPDATE user:yqjhe8r14hmnobsqgaci CONTENT { email: 'student1@student.sust.edu', id: user:yqjhe8r14hmnobsqgaci, password: '$argon2id$v=19$m=19456,t=2,p=1$8HsAe25TMokJCcSC+JWe/A$EYDlkALuunKiIEdz35Ti6UK2TG5hbYiHkzs9hcB0cf0' };
UPDATE user:yycl51qmt5pz4gn798ct CONTENT { email: 'amelia01@student.sust.edu', id: user:yycl51qmt5pz4gn798ct, password: '$argon2id$v=19$m=19456,t=2,p=1$G7HE7CxuzYIEOM4tfTzozQ$syRVMt8PCP9TSC2Ab678gIz+HGs16Ease/YgZQ4Fo1M' };

-- ------------------------------
-- TRANSACTION
-- ------------------------------

COMMIT TRANSACTION;

