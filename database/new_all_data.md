### department
```sql
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
    code: 333,
    id: string::trim('PHY'),
    letter_code: string::trim('PHY'),
    name: string::trim('Physics'),
    building: string::trim('A'),
    floor: 1,
    minor_course_code: string::trim('P')
}

create department content {
    code: 332,
    id: string::trim('MAT'),
    letter_code: string::trim('MAT'),
    name: string::trim('Mathematics'),
    building: string::trim('C'),
    floor: 4,
    minor_course_code: string::trim('M')
}
```

### creating course to ``CSE`` and relating them
```sql

--relate department:CSE->offers->course:CSE330 set semester=5, year=2023;

--relate department:CSE->offers->course:CSE222 set semester=3, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE210 CONTENT {
    credit: 3,
    name: 'Data Structures and Algorithms',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Introduction to Algorithms',
            topics: [
                'Analysis of algorithms',
                'Asymptotic notation',
            ],
        },
        {
            title: 'Data Structures',
            topics: [
                'Arrays',
                'Linked lists',
                'Treess',
            ],
        },
    ],
})) set semester=1, year=2023;


relate department:CSE->offers->( select id from ( CREATE course:CSE211 CONTENT {
    credit: 3,
    name: 'Algorithm Design',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Greedy Algorithms',
            topics: [
                'Activity selection',
                'Huffman coding',
            ],
        },
        {
            title: 'Dynamic Programming',
            topics: [
                'Memoization',
                'Longest common subsequence',
            ],
        },
    ],
})) set semester=3, year=2023;


relate department:CSE->offers->( select id from ( CREATE course:CSE211 CONTENT {
    credit: 3,
    name: 'Algorithm Design',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Greedy Algorithms',
            topics: [
                'Activity selection',
                'Huffman coding',
            ],
        },
        {
            title: 'Dynamic Programming',
            topics: [
                'Memoization',
                'Longest common subsequence',
            ],
        },
    ],
})) set semester=3, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE312 CONTENT {
    credit: 3,
    name: 'Database Management Systems',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Introduction to Databases',
            topics: [
                'Database systems',
                'ER modeling',
            ],
        },
        {
            title: 'Query Languages',
            topics: [
                'S Q L ',
                'Normalization',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE313 CONTENT {
    credit: 3,
    name: 'Operating Systems',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Introduction to OS',
            topics: [
                'Processes',
                'Memory management',
            ],
        },
        {
            title: 'File Systems',
            topics: [
                'Disk management',
                'File organization',
            ],
        },
    ],
})) set semester=5, year=2023;


relate department:CSE->offers->( select id from ( CREATE course:CSE314 CONTENT {
    credit: 3,
    name: 'Computer Networks',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Introduction to Networking',
            topics: [
                'Protocols',
                'Network layers',
            ],
        },
        {
            title: 'Network Security',
            topics: [
                'Cryptography',
                'Firewalls',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE315 CONTENT {
    credit: 3,
    name: 'Software Engineering',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Software Development Life Cycle',
            topics: [
                'Requirements engineering',
                'Agile methodologies',
            ],
        },
        {
            title: 'Quality Assurance',
            topics: [
                'Testing strategies',
                'Code review',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE221 CONTENT {
    credit: 2,
    name: 'Data Structures Lab',
    department: department:CSE,
    type: 'Lab',
    syllabus: [
        {
            title: 'Basic Data Structures',
            topics: [
                'Arrays',
                'Linked lists',
            ],
        },
        {
            title: 'Searching and Sorting',
            topics: [
                'Binary search',
                'Bubble sort',
            ],
        },
    ],
})) set semester=1, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE322 CONTENT {
    credit: 2,
    name: 'Database Lab',
    department: department:CSE,
    type: 'Lab',
    syllabus: [
        {
            title: 'SQL Queries',
            topics: [
                'SELECT statements',
                'Joins ',
            ],
        },
        {
            title: 'Database Design',
            topics: [
                'Entity-relationship diagrams',
                'Normalization',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE323 CONTENT {
    credit: 2,
    name: 'Operating Systems Lab',
    department: department:CSE,
    type: 'Lab',
    syllabus: [
        {
            title: 'Processes and Threads',
            topics: [
                'Forking',
                'Thread synchronization',
            ],
        },
        {
            title: 'File Systems',
            topics: [
                'File manipulation',
                'Directory operations',
            ],
        },
    ],
})) set semester=5, year=2023;


relate department:CSE->offers->( select id from ( CREATE course:CSE324 CONTENT {
    credit: 2,
    name: 'Networking Lab',
    department: department:CSE,
    type: 'Lab',
    syllabus: [
        {
            title: 'Socket Programming',
            topics: [
                'Client-server communication',
                'UDP and TCP sockets',
            ],
        },
        {
            title: 'Network Configuration',
            topics: [
                'IP addressing',
                'Routing',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE325 CONTENT {
    credit: 2,
    name: 'Software Engineering Lab',
    department: department:CSE,
    type: 'Lab',
    syllabus: [
        {
            title: 'Requirements Gathering',
            topics: [
                'Interviews',
                'Use cases',
            ],
        },
        {
            title: 'Testing and Debugging',
            topics: [
                'Unit testing',
                'Debugging techniques',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE230 CONTENT {
    credit: <float>1.5,
    name: 'Algorithm Implementation Lab',
    department: department:CSE,
    type: 'Lab',
    syllabus: [
        {
            title: 'Sorting Algorithms',
            topics: [
                'Bubble sort',
                'Insertion sort',
            ],
        },
        {
            title: 'Data Structures Implementation',
            topics: [
                'Linked lists',
                'Stacks and queues',
            ],
        },
    ],
})) set semester=3, year=2023;

relate department:CSE->offers->( select id from ( CREATE course:CSE101 CONTENT {
    credit: 2,
    name: 'Introduction to Programming',
    department: department:CSE,
    type: 'Theory',
    syllabus: [
        {
            title: 'Programming Basics',
            topics: [
                'Variables and data types',
                'Control structures',
            ],
        },
        {
            title: 'Programming Logic',
            topics: [
                'Loops ',
                'Functions',
            ],
        },
    ],
})) set semester=1, year=2023;



```

### creating course to ``PHY`` and relating them

```sql
relate department:PHY->offers->( select id from ( CREATE course:PHY101 CONTENT {
    credit: 3,
    name: 'Classical Mechanics',
    department: department:PHY,
    type: 'Theory',
    syllabus: [
        {
            title: 'Newtonian Mechanics',
            topics: [
                'Kinematics',
                'Forces and laws of motion',
            ],
        },
        {
            title: 'Work, Energy, and Momentum',
            topics: [
                'Conservation laws',
                'Collisions',
            ],
        },
    ],
})) set semester=1, year=2023;

relate department:PHY->offers->( select id from ( CREATE course:PHY202 CONTENT {
    credit: 3,
    name: 'Electromagnetism',
    department: department:PHY,
    type: 'Theory',
    syllabus: [
        {
            title: 'Electric Fields and Forces',
            topics: [
                'Coulomb`s law',
                'Electric potential',
            ],
        },
        {
            title: 'Magnetic Fields and Induction',
            topics: [
                'Biot-Savart law',
                'Faraday`s law',
            ],
        },
    ],
})) set semester=3, year=2023;

relate department:PHY->offers->( select id from ( CREATE course:PHY303 CONTENT {
    credit: 3,
    name: 'Quantum Mechanics I',
    department: department:PHY,
    type: 'Theory',
    syllabus: [
        {
            title: 'Wave Particle Duality',
            topics: [
                'Wave functions',
                'Uncertainty principle',
            ],
        },
        {
            title: 'Quantum Operators',
            topics: [
                'Observables',
                'Eigenstates',
            ],
        },
    ],
})) set semester=5, year=2023;

relate department:PHY->offers->( select id from ( CREATE course:PHY204 CONTENT {
    credit: 3,
    name: 'Thermodynamics and Statistical Mechanics',
    department: department:PHY,
    type: 'Theory',
    syllabus: [
        {
            title: 'Laws of Thermodynamics',
            topics: [
                'Work, heat, energy',
                'Entropy',
            ],
        },
        {
            title: 'Statistical Mechanics',
            topics: [
                'Microcanonical ensemble',
                'Entropy and probability',
            ],
        },
    ],
})) set semester=3, year=2023;

```


### creating course to ``MAT`` and relating them
```sql
relate department:MAT->offers->( select id from ( CREATE course:MAT101 CONTENT {
    credit: 3,
    name: 'Calculus I',
    department: department:MAT,
    type: 'Theory',
    syllabus: [
        {
            title: 'Limits and Continuity',
            topics: [
                'Limit definition',
                'Continuity',
            ],
        },
        {
            title: 'Differentiation',
            topics: [
                'Derivative rules',
                'Applications',
            ],
        },
    ],
})) set semester=1, year=2023;

relate department:MAT->offers->( select id from ( CREATE course:MAT204 CONTENT {
    credit: 3,
    name: 'Differential Equations',
    department: department:MAT,
    type: 'Theory',
    syllabus: [
        {
            title: 'First Order Differential Equations',
            topics: [
                'Separable equations',
                'Exact equations',
            ],
        },
        {
            title: 'Second Order Differential Equations',
            topics: [
                'Homogeneous equations',
                'Undetermined coefficients',
            ],
        },
    ],
})) set semester=3, year=2023;

relate department:MAT->offers->( select id from ( CREATE course:MAT205 CONTENT {
    credit: 3,
    name: 'Probability and Statistics',
    department: department:MAT,
    type: 'Theory',
    syllabus: [
        {
            title: 'Probability Basics',
            topics: [
                'Sample spaces',
                'Conditional probability',
            ],
        },
        {
            title: 'Statistical Inference',
            topics: [
                'Hypothesis testing',
                'Confidence intervals',
            ],
        },
    ],
})) set semester=3, year=2023;


```

### creating teacher in `cse`
```sql

CREATE teacher:2019445088 CONTENT {
    name: string::trim('Sabrina Rahman'),
    department: department:CSE,
    designation: string::trim('Assistant Professor'),
    email: {
        personal: string::trim('sabrina1234@gmail.com'),
        academic: string::trim('sabrina88@sust.edu'),
    },
    gender: string::lowercase('female'),
    blood_group: string::trim('A-'),
    personal: {
        father: string::trim('Abdul Rahman'),
        mother: string::trim('Nasrin Begum'),
        birthday: "1988-03-15T14:30:00Z",
        phone: 01991234567,
        hometown: string::trim('Dhaka'),
    },
};

create user set email = 'sabrina88@sust.edu', password = crypto::argon2::generate("root");
relate (select id from user where email = 'sabrina88@sust.edu') -> login -> teacher:2019445088;


CREATE teacher:2019667001 CONTENT {
    name: string::trim('Fariha Rahman'),
    department: department:CSE,
    designation: string::trim('Lecturer'),
    email: {
        personal: string::trim('fariha_r@gmail.com'),
        academic: string::trim('fariha27@sust.edu'),
    },
    gender: string::lowercase('female'),
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Iqbal Rahman'),
        mother: string::trim('Nusrat Rahman'),
        birthday: "1996-09-05T09:45:00Z",
        phone: 01651234567,
        hometown: string::trim('Khulna'),
    },
};


relate (select id from (create user set email = 'fariha27@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2019667001;


CREATE teacher:2019773019 CONTENT {
    name: string::trim('Rahim Khan'),
    department: department:CSE,
    designation: string::trim('Professor'),
    email: {
        personal: string::trim('rahim.khan@gmail.com'),
        academic: string::trim('rahim123@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('AB-'),
    personal: {
        father: string::trim('Nasir Khan'),
        mother: string::trim('Farida Khanam'),
        birthday: "1960-05-10T10:30:00Z",
        phone: 01767890123,
        hometown: string::trim('Rajshahi'),
    },
};


relate (select id from (create user set email = 'rahim123@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2019773019;


CREATE teacher:2019886030 CONTENT {
    name: string::trim('Aisha Islam'),
    department: department:CSE,
    designation: string::trim('Assistant Professor'),
    email: {
        personal: string::trim('aisha.islam@gmail.com'),
        academic: string::trim('aisha55@sust.edu'),
    },
    gender: string::lowercase('female'),
    blood_group: string::trim('A+'),
    personal: {
        father: string::trim('Kamal Islam'),
        mother: string::trim('Laila Islam'),
        birthday: "1985-07-25T15:20:00Z",
        phone: 01990001111,
        hometown: string::trim('Sylhet'),
    },
};


relate (select id from (create user set email = 'aisha55@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2019886030;


```

### creating teacher in `phy`
```sql
CREATE teacher:2020005001 CONTENT {
    name: string::trim('Mohammed Ali'),
    department: department:PHY,
    designation: string::trim('Assistant Professor'),
    email: {
        personal: string::trim('mohammed.ali@gmail.com'),
        academic: string::trim('ali2001@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('O-'),
    personal: {
        father: string::trim('Abdullah Khan'),
        mother: string::trim('Farida Begum'),
        birthday: "1985-09-10T10:45:00Z",
        phone: 01761234567,
        hometown: string::trim('Chittagong'),
    },
};

relate (select id from (create user set email = 'ali2001@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020005001;

CREATE teacher:2020117002 CONTENT {
    name: string::trim('Sara Ahmed'),
    department: department:PHY,
    designation: string::trim('Lecturer'),
    email: {
        personal: string::trim('sara.ahmed@gmail.com'),
        academic: string::trim('sara02@sust.edu'),
    },
    gender: string::lowercase('female'),
    blood_group: string::trim('B-'),
    personal: {
        father: string::trim('Rahim Ahmed'),
        mother: string::trim('Nadia Rahman'),
        birthday: "1994-06-12T14:15:00Z",
        phone: 01887654322,
        hometown: string::trim('Dhaka'),
    },
};

relate (select id from (create user set email = 'sara02@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020117002;

CREATE teacher:2020228003 CONTENT {
    name: string::trim('Ali Khan'),
    department: department:PHY,
    designation: string::trim('Professor'),
    email: {
        personal: string::trim('ali.khan@gmail.com'),
        academic: string::trim('alikhan03@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('A+'),
    personal: {
        father: string::trim('Nasir Khan'),
        mother: string::trim('Fahima Khanam'),
        birthday: "1978-03-20T12:30:00Z",
        phone: 01651234568,
        hometown: string::trim('Rajshahi'),
    },
};

relate (select id from (create user set email = 'alikhan03@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020228003;

```

### creating teacher in `mat`
```sql
CREATE teacher:2020553001 CONTENT {
    name: string::trim('Rahim Khan'),
    department: department:MAT,
    designation: string::trim('Assistant Professor'),
    email: {
        personal: string::trim('rahimkhan@gmail.com'),
        academic: string::trim('rahim01@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('O+'),
    personal: {
        father: string::trim('Nasir Khan'),
        mother: string::trim('Farida Khanam'),
        birthday: "1985-10-10T10:45:00Z",
        phone: 01761234568,
        hometown: string::trim('Rajshahi'),
    },
};

relate (select id from (create user set email = 'rahim01@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020553001;


CREATE teacher:2020665002 CONTENT {
    name: string::trim('Sara Ahmed'),
    department: department:MAT,
    designation: string::trim('Lecturer'),
    email: {
        personal: string::trim('saraahmed@gmail.com'),
        academic: string::trim('sara@sust.edu'),
    },
    gender: string::lowercase('female'),
    blood_group: string::trim('B-'),
    personal: {
        father: string::trim('Rahim Ahmed'),
        mother: string::trim('Nadia Rahman'),
        birthday: "1994-06-12T14:15:00Z",
        phone: 01887654312,
        hometown: string::trim('Dhaka'),
    },
};

relate (select id from (create user set email = 'sara@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020665002;

CREATE teacher:2020777003 CONTENT {
    name: string::trim('Ali Khan'),
    department: department:MAT,
    designation: string::trim('Professor'),
    email: {
        personal: string::trim('alikhan@gmail.com'),
        academic: string::trim('alikhan@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('A+'),
    personal: {
        father: string::trim('Nasir Khan'),
        mother: string::trim('Fahima Khanam'),
        birthday: "1978-03-20T12:30:00Z",
        phone: 01651234566,
        hometown: string::trim('Rajshahi'),
    },
};

relate (select id from (create user set email = 'alikhan@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020777003;


CREATE teacher:2020991005 CONTENT {
    name: string::trim('Kamal Ahmed'),
    department: department:MAT,
    designation: string::trim('Assistant Professor'),
    email: {
        personal: string::trim('kamal.ahmed@gmail.com'),
        academic: string::trim('kamal05@sust.edu'),
    },
    gender: string::lowercase('male'),
    blood_group: string::trim('B+'),
    personal: {
        father: string::trim('Ahmed Khan'),
        mother: string::trim('Farida Ahmed'),
        birthday: "1990-11-25T14:30:00Z",
        phone: 01990001110,
        hometown: string::trim('Sylhet'),
    },
};

relate (select id from (create user set email = 'kamal05@sust.edu', password = crypto::argon2::generate("root"))) -> login -> teacher:2020991005;

```

### relating teacher with courses for `cse`
```sql
relate teacher:2019886030->teaches->(select <-offers.id as id from course:CSE230 split id) set guest_dept='';

relate teacher:2019773019->teaches->(select <-offers.id as id from course:CSE222 split id) set guest_dept='';

relate teacher:2019667001->teaches->(select <-offers.id as id from course:CSE221 split id) set guest_dept='';

relate teacher:2019559045->teaches->(select <-offers.id as id from course:CSE211 split id) set guest_dept='';

relate teacher:2019445088->teaches->(select <-offers.id as id from course:CSE210 split id) set guest_dept='';

relate teacher:2019331073->teaches->(select <-offers.id as id from course:CSE101 split id) set guest_dept='';

relate teacher:2016331033->teaches->(select <-offers.id as id from course:CSE312 split id) set guest_dept='';

relate teacher:2019445088->teaches->(select <-offers.id as id from course:CSE313 split id) set guest_dept='';

relate teacher:2019667001->teaches->(select <-offers.id as id from course:CSE314 split id) set guest_dept='';

relate teacher:2019886030->teaches->(select <-offers.id as id from course:CSE315 split id) set guest_dept='';

relate teacher:2019445088->teaches->(select <-offers.id as id from course:CSE330 split id) set guest_dept='';
```


### relating teacher with courses for `phy`
```sql
-- PHY Department Queries

-- Query 1
relate teacher:2020005001->teaches->(select <-offers.id as id from course:PHY101 split id) set guest_dept='';

-- Query 2
relate teacher:2020117002->teaches->(select <-offers.id as id from course:PHY202 split id) set guest_dept='';

-- Query 3
relate teacher:2020228003->teaches->(select <-offers.id as id from course:PHY204 split id) set guest_dept='';

-- Query 4
relate teacher:2020228003->teaches->(select <-offers.id as id from course:PHY303 split id) set guest_dept='';

-- Query 5
relate teacher:2020005001->teaches->(select <-offers.id as id from course:PHY101 split id) set guest_dept='';

```

### relating teacher with courses for `mat`
```sql
-- MAT Department Queries

-- Query 1
relate teacher:2020553001->teaches->(select <-offers.id as id from course:MAT101 split id) set guest_dept='';

-- Query 2
relate teacher:2020665002->teaches->(select <-offers.id as id from course:MAT204 split id) set guest_dept='';

-- Query 3
relate teacher:2020777003->teaches->(select <-offers.id as id from course:MAT205 split id) set guest_dept='';

-- Query 4
relate teacher:2020991005->teaches->(select <-offers.id as id from course:MAT101 split id) set guest_dept='';

```

### relating student with course for `cse`

```sql
relate student:2019331013->takes->course:CSE221 
set semester=1, year=2020;

relate student:2019331013->takes->course:CSE221 set semester=1, year=2020;

relate student:2019331073->takes->course:CSE313 set semester=5, year=2020;

relate student:2022336011->takes->course:CSE101 set semester=1, year=2021;

relate student:2019331013->takes->course:CSE312 set semester=5, year=2021;

relate student:2019331073->takes->course:CSE222 set semester=3, year=2022;

relate student:2022336011->takes->course:CSE330 set semester=5, year=2022;

relate student:2019331013->takes->course:CSE314 set semester=5, year=2023;

relate student:2019331073->takes->course:CSE222 set semester=3, year=2023;

relate student:2022336011->takes->course:CSE324 set semester=5, year=2024;

relate student:2019331013->takes->course:CSE210 set semester=5, year=2024;

```

