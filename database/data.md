```sql
CREATE student:2022336011 CONTENT {
    name: string::trim('Amelia Harris'),
    department: department:CSE,
    email: {
       personal: string::trim('amelia.harris@example.com'),
       academic: string::trim('amelia01@student.sust.edu'),
    },
    gender: string::lowercase('female'),
    session: 2022,
    current_semester: 1,
    blood_group: string::trim('AB-'),
    privilege: string::trim('Student'),
    personal: {
        father: string::trim('Christopher Harris'),
        mother: string::trim('Jessica Harris'),
        birthday: "2002-03-20T09:45:00Z",
        phone: 01743210987,
        hometown: string::trim('Atlanta'),
    },
    result: {
        cgpa: <float>3.6,
        grade: string::trim('A-'),
        total_credit: 40,
    },
};
create user set email = 'amelia01@student.sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'amelia01@student.sust.edu') -> login -> student:2022336011;


CREATE student:2023119144 CONTENT {
    name: string::trim('Amina Islam'),
    department: department:PHY,
    email: {
       personal: string::trim('amina.islam@example.com'),
       academic: string::trim('amina05@student.sust.edu'),
    },
    gender: string::lowercase('female'),
    session: 2022,
    current_semester: 5,
    blood_group: string::trim('O+'),
    personal: {
        father: string::trim('Mohammed Islam'),
        mother: string::trim('Fatima Islam'),
        birthday: "2001-10-08T10:00:00Z",
        phone: 01732109877,
        hometown: string::trim('Khulna'),
    },
    result: {
        cgpa: <float>3.9,
        grade: string::trim('A'),
        total_credit: <float>140,
    },
};
create user set email = 'amina05@student.sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'amina05@student.sust.edu') -> login -> student:2023119144;



CREATE student:2022998101 CONTENT {
    name: string::trim('Arjun Chowdhury'),
    department: department:SWE,
    email: {
       personal: string::trim('arjun.chowdhury@example.com'),
       academic: string::trim('arjun04@student.sust.edu'),
    },
    gender: string::lowercase('male'),
    session: 2022,
    current_semester: 4,
    blood_group: string::trim('B-'),
    privilege: string::trim('Student'),
    personal: {
        father: string::trim('Rahim Chowdhury'),
        mother: string::trim('Nasreen Chowdhury'),
        birthday: "2001-07-12T09:15:00Z",
        phone: 01743210989,
        hometown: string::trim('Rajshahi'),
    },
    result: {
        cgpa: <float>3.2,
        grade: string::trim('B'),
        total_credit: <float>100,
    },
};
create user set email = 'arjun04@student.sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'arjun04@student.sust.edu') -> login -> student:2022998101;


CREATE student:2022887055 CONTENT {
    name: string::trim('Sophia Ahmed'),
    department: department:ECO,
    email: {
       personal: string::trim('sophia.ahmed@example.com'),
       academic: string::trim('sophia03@student.sust.edu'),
    },
    gender: string::lowercase('female'),
    session: 2022,
    current_semester: 2,
    blood_group: string::trim('A-'),
    privilege: string::trim('Student'),
    personal: {
        father: string::trim('Kamal Ahmed'),
        mother: string::trim('Sara Ahmed'),
        birthday: "2001-09-10T14:00:00Z",
        phone: 01754321098,
        hometown: string::trim('Chittagong'),
    },
    result: {
        cgpa: <float>3.8,
        grade: string::trim('A'),
        total_credit: <float>60,
    },
};
create user set email = 'sophia03@student.sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'sophia03@student.sust.edu') -> login -> student:2022887055;


CREATE student:2022447122 CONTENT {
    name: string::trim('Ethan Rahman'),
    department: department:PHY,
    email: {
       personal: string::trim('ethan.rahman@example.com'),
       academic: string::trim('ethan02@student.sust.edu'),
    },
    gender: string::lowercase('male'),
    session: 2022,
    current_semester: 3,
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Ahmed Rahman'),
        mother: string::trim('Nadia Rahman'),
        birthday: "2001-05-15T11:30:00Z",
        phone: 01732109876,
        hometown: string::trim('Dhaka'),
    },
    result: {
        cgpa: <float>3.5,
        grade: string::trim('B+'),
        total_credit: <float>75,
    },
};
create user set email = 'ethan02@student.sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'ethan02@student.sust.edu') -> login -> student:2022447122;

```
