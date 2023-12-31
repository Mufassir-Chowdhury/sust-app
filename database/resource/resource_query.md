```sql
DEFINE TABLE resource SCHEMAFULL;

define field posted_by on resource type record(student, admin, teacher) assert $value!=none;

define field title on resource type string
assert $value !=none;

define field link on resource type string 
assert $value != none;

define field date on resource type datetime
assert $value != none;
```

### Adding a resource
```sql
relate course:CSE222->has->(select id from (create resource content {
    posted_by: student:2019331073,
    title: 'Introduction to surrealdb',
    link: 'https://www.youtube.com/watch?v=C7WFwgDRStM&ab_channel=Fireship',
    date: time::now()
})) content {
    type: 'Video',
    year: '2023'
}
```

### fetching resource
```sql
select *, posted_by.name as posted_by
from (select value ->(has where year='2023' and type='Video')->resource as resource 
from course:CSE222 
fetch student,admin,teacher)
```

