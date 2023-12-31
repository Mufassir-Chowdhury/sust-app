
```sql
DEFINE TABLE routine SCHEMAFULL;

Define field time on routine type number
assert $value != none and $value >= 8 and $value<=17;

Define field duration on routine type string 
assert $value inside ['1hr','2hr', 3hr];

Define field room on routine type string
assert $value != none; 

Define field building on routine type string
assert $value != none; 



create routine content {
    time: 9,
    duration: '1hr',
    room: 'G-2',
    building: 'IICT'
};

create routine content {
    time: 10,
    duration: '2hr',
    room: '304',
    building: 'IICT'
};
```

### Relate a course with a routine
```sql 
relate course:CSE330->follows->(select id from (create routine content {
    time: 14,
    duration: '1hr',
    room: 'G-2',
    building: 'IICT'
})) set day='Wednesday', year='2023';

```

### find a routine  of a year of a course
```sql
--better
select course_code as code, 
<-(offers where year=2019 and semester=12)<-teaches<-teacher.name as name,
(select day, 
->routine.* as routine
from (select value  ->(follows where year=2023)
from course:CSE222)) as class from course:CSE222 split name;

--normal
select <-course.course_code as code,
<-course<-(offers where year=2019 and semester=12)<-teaches<-teacher.name as name,
day, 
->routine.* as routine
from (select value  ->(follows where year=2023)
from course:CSE222) split code, name;
```

### find routine of a course in a perticular day
```sql
select ->(follows where day='Sunday' and year=2023)->routine.* as routine
from course:CSE222
```



