```sql
select id, name 
from (select value ->(offers 
                    where semester = 12 and year = 2019)->course 
from department:CSE);


let $one = select value id from (select value ->(offers where semester = 12 and year = 2019)->course from department:CSE);

let $two = select value id from (select value ->(takes where semester=12 and year = 2019)->course from student:2019331073);

select id,name from array::difference($one, $two);

relate student:2019331073->takes->course:CSE330 set semester=12, year=2019;


```

### Course Taken by a student
```
select ->takes->course.name as takenCourseList from student:2019331013;
```

### Course Teaches by a teacher
```
select ->teaches->offers->course.name as teachesCourseList from teacher:2019331073;
```


