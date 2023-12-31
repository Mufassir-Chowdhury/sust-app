### duplicate `id`
#### query
```sql
create course:cse133 content {
    ...,
};
```
#### result
```json
[
  {
    "time": "53.7µs",
    "status": "ERR",
    "detail": "Database record `course:cse133` already exists"
  }
]
```

### duplicate `course_title`
#### query
```sql
create course:cse134 content {
    course_title : string::trim('Structured Programming Language'),
    ...,
};
```
#### result
```json
[
  {
    "time": "6.5656ms",
    "status": "ERR",
    "detail": "Database index `course_title` already contains 'Structured Programming Language', with record `course:cse134`"
  }
]
```

### Wrong input in `course_title` field
#### query
```sql
create course:cse133 content {
    course_title : string::trim(''|'math'|'Algorithm 2'|'phy.'),
    ...,
};
```
#### result
```json
[
  {
    "time": "261.1µs",
    "status": "ERR",
    "detail": "Found ''|'math'|'Algorithm 2'|'phy.' for field `course_title`, with record `course:cse133`, but field must conform to: $value != NONE AND $value = /^[A-Za-z ]+$/ AND string::len($value) > 5"
  }
]
```

### Wrong input in `credit` field
#### query
```sql
create course:cse133 content {
    credit : 3.5|0|-2,
    ...,
};
```
#### result
```json
[
  {
    "time": "1.1832ms",
    "status": "ERR",
    "detail": "Found 3.5|0|-2 for field `credit`, with record `course:cse133`, but field must conform to: $value != NONE AND $value > 0 AND $value <= 3"
  }
]
```

### Wrong input in `type` field
#### query
```sql
create course:cse133 content {
    type : string::trim('Theoy'|'012'|'theory'),
    ...,
};
```
#### result
```json
[
  {
    "time": "380.3µs",
    "status": "ERR",
    "detail": "Found 'Theoy'|'012'|'theory' for field `type`, with record `course:cse133`, but field must conform to: $value INSIDE ['Lab', 'Theory', 'Thesis']"
  }
]
```