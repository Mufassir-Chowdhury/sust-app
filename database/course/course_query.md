### To create the table `course`
```sql
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
    and $value = /^[A-Za-z ,&]+$/
    and string::len($value) > 5;

define field syllabus.*.topics on course type array;
define field syllabus.*.topics.* on course type string
    assert $value != none
    and string::len($value) > 5;
```

### `course` adding format
```sql
create course:`course_code` content {
    course_tile : string::trim($value (string)),
    credit : $value (float),
    type : $value (string),
    department: string::concat('department:', $value(string)),
    syllabus : [
        {
            title: string::trim($value (string)),
            topics: [
                string::trim($value (string)),
                string::trim($value (string)),
            ],
        },
    ]
};
```

### Example to insert some values
```sql
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
}


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
```


### update e course
```sql
update course:CSE222 merge {
	item : {
		item : ...
	}
}
```

### all the student and teacher assigned to a course
```sql
select name,<-takes<-student.name as assigned_students,
array::distinct(<-offers<-teaches<-teacher.name) as assigned_teachers
from course
```