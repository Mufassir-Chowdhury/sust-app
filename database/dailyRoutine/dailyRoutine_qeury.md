```sql
DEFINE TABLE dailyroutine SCHEMAFULL;

Define field time on dailyroutine type number
assert $value != none and $value >= 8 and $value<=18;

Define field duration on dailyroutine type string 
assert $value inside ['1hr','2hr', 3hr];

Define field room on dailyroutine type string
assert $value != none; 

Define field building on dailyroutine type string
assert $value != none; 

define field date on dailyroutine type datetime
assert $value != none;
```


### relate a course with a dailyroutine
```sql
relate course:CSE330->having->(select id 
from (create dailyroutine content {
    time: 14,
    duration: '1hr',
    room: 'G-2',
    building: 'IICT',
    date: time::now()
}));
```

