### duplicate `id`
#### query
```sql
CREATE student CONTENT {
    id : 2019331073,
    ...,
};
```
#### result
```json
[
  {
    "time": "147.8µs",
    "status": "ERR",
    "detail": "Database record `student:2019331073` already exists"
  }
]
```

### duplicate `email.personal`

#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    email: {
       personal: string::trim('mac22214u@gmail.com'),
       ...,
    },
    ...,
};
```

#### result
```json
[
  {
    "time": "1.2667ms",
    "status": "ERR",
    "detail": "Database index `personal_email` already contains 'mac22214u@gmail.com', with record `student:2019331074`"
  }
]
```

### duplicate `email.academic`

#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    email: {
       ...,
       academic:string::trim( 'mufassir73@student.sust.edu'),
    },
    ...,
};
```

#### result
```json
[
  {
    "time": "1.5647ms",
    "status": "ERR",
    "detail": "Database index `academic_email` already contains 'mufassir73@student.sust.edu', with record `student:2019331074`"
  }
]
```

### duplicate `personal.phone`

#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    personal: {
        ...,
        phone: 01771144308,
    },
    ...,
};
```

#### result
```json
[
  {
    "time": "1.3203ms",
    "status": "ERR",
    "detail": "Database index `phone` already contains 1771144308, with record `student:2019331074`"
  }
]
```


### wrong input in `name`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    name: string::trim('Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y'),
    ...,
};
```
#### result
```json
[
  {
    "time": "899.3µs",
    "status": "ERR",
    "detail": "Found 'Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y' for field `name`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3"
  }
]
```

### `department` will be detect automatically

### wrong input in `email.personal`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    email: {
       personal: string::trim('mac22214gmail.com'|''|'1234'|'@gmail.com'),
       ...,
    },
    ...,
};
```
#### result
```json
[
  {
    "time": "395µs",
    "status": "ERR",
    "detail": "Found 'mac22214gmail.com'|''|'1234'|'@gmail.com' for field `email.personal`, with record `student:2019331074`, but field must conform to: $value != NONE AND is::email($value)"
  }
]
```

### wrong input in `email.academic`
#### query
```sql
--delete student;
CREATE student CONTENT {
    id : 2019331074,
    email: {
       academic:string::trim( 'mufassir74@studet.sust.edu'|''|'1234'|'asanul@student.sust.edu'|'13@student.sust.edu'),
       ...,
    },
    ..., 
};
```
#### result
```json
[
  {
    "time": "486.2µs",
    "status": "ERR",
    "detail": "Found 'mufassir74@studet.sust.edu'|''|'1234'|'asanul@student.sust.edu'|'13@student.sust.edu' for field `email.academic`, with record `student:2019331074`, but field must conform to: $value != NONE AND is::email($value) AND parse::email::host($value) = 'student.sust.edu' AND parse::email::user($value) >= 5 AND is::numeric(string::slice(parse::email::user($value), string::len(parse::email::user($value)) - 2, 2)) AND is::alpha(string::slice(parse::email::user($value), 0, string::len(parse::email::user($value)) - 2))"
  }
]
```

### wrong input in `gender`
#### query
```sql
--delete student;
CREATE student CONTENT {
    id : 2019331074,
    gender: string::lowercase('mle'||''|'1234'),
    ...,
};
```
#### result
```json
[
  {
    "time": "779.4µs",
    "status": "ERR",
    "detail": "Found 'mle'||''|'1234' for field `gender`, with record `student:2019331074`, but field must conform to: $value INSIDE ['male', 'female']"
  }
]
```

### wrong input in `session`
#### query (type same but out of conditions)
```sql
CREATE student CONTENT {
    id : 2019331074,
    session: 3019|-123|2024|1979|2000.6,
    ...,
};
```
#### result
```json
[
  {
    "time": "1.0755ms",
    "status": "ERR",
    "detail": "Found 3019|-123|2024|1979|2000.6 for field `session`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 1980 AND $value <= time::year() AND math::round($value) = $value"
  }
]
```
#### query (type different)
```sql
--delete student;
CREATE student CONTENT {
    id : 2019331074,
    ...,
    session: 'hello'|hiiii|[],
};
```
#### result
```json
[
  {
    "time": "368µs",
    "status": "OK",
    "result": []
  },
  {
    "time": "1.0841ms",
    "status": "ERR",
    "detail": "Found 0|NONE|0 for field `session`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 1980 AND $value <= time::year() AND math::round($value) = $value"
  }
]
```

### wrong input in `current_semester`
#### query (type same but out of conditions)
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    current_semester: 0|11|11.5|3.4|-2,
    
};
```
#### result
```json
[
  {
    "time": "852.5µs",
    "status": "ERR",
    "detail": "Found 0|11|11.5|3.4|-2 for field `current_semester`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 1 AND $value <= 8 AND math::round($value) = $value"
  }
]
```

#### query (different type)
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    current_semester: 'hi'|hi|[],
    
};
```
#### result
```json
[
  {
    "time": "899.3µs",
    "status": "ERR",
    "detail": "Found 0|NONE|0 for field `current_semester`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 1 AND $value <= 8 AND math::round($value) = $value"
  }
]
```

### wrong input in `blood_group`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    blood_group: string::trim('B'|'123'|'b+'),
    
};
```
#### result
```json
[
  {
    "time": "6.5455ms",
    "status": "ERR",
    "detail": "Found 'B'|'123'|'b+' for field `blood_group`, with record `student:2019331074`, but field must conform to: $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-']"
  }
]
```

### wrong input in `privilege`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    privilege: string::trim('C'), 
};
```
#### result
```json
[
  {
    "time": "1.4276ms",
    "status": "ERR",
    "detail": "Found 'C' for field `privilege`, with record `student:2019331074`, but field must conform to: $value = NONE OR ($value = /^[A-Za-z. ]+$/ AND string::len($value) > 1)"
  }
]
```

### wrong input in `personal.father`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    personal : {
        father : string::trim('Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y'),
        ...,
    }
};
```
#### result
```json
[
  {
    "time": "899.3µs",
    "status": "ERR",
    "detail": "Found 'Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y' for field `personal.father`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3"
  }
]
```

### wrong input in `personal.mother`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    personal : {
        mother : string::trim('Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y'),
        ...,
    }
};
```
#### result
```json
[
  {
    "time": "899.3µs",
    "status": "ERR",
    "detail": "Found 'Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y' for field `personal.mother`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3"
  }
]
```

### wrong input in `personal.birthday`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    personal: {
        ...,
        birthday: 2001-07-10,
    },
};
```
#### result
```json
[
  {
    "time": "683.4µs",
    "status": "ERR",
    "detail": "Found '2023-04-24T16:20:27.412492Z' for field `personal.birthday`, with record `student:2019331074`, but field must conform to: $value != NONE AND time::year($value) < time::year() - 15"
  }
]
```

### wrong input in `personal.phone`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    personal: {
        ...,
        phone: 771144309|2774782347,
    },
};
```
#### result
```json
[
  {
    "time": "666.1µs",
    "status": "ERR",
    "detail": "Found 771144309|2774782347 for field `personal.phone`, with record `student:2019331074`, but field must conform to: $value != NONE AND math::round($value) = $value AND string::startsWith(<string> $value, '1') AND string::len(<string> $value) = 10"
  }
]
```

### wrong input in `personal.hometown`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    personal: {
        ...,
        hometown: string::trim(''|'hi'|'rajshahi4'|'sylhet.'|'1234'),
    },
};
```
#### result
```json
[
  {
    "time": "622.8µs",
    "status": "ERR",
    "detail": "Found ''|'hi'|'rajshahi4'|'sylhet.'|'1234' for field `personal.hometown`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z, ]+$/ AND string::len($value) >= 3"
  }
]
```

### wrong input in `result.cgpa`
#### query (type same but out of conditions)
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    result: {
        cgpa: 4.5|-1.3|45,
        ...,
    },
    
};
```
#### result
```json
[
  {
    "time": "851µs",
    "status": "ERR",
    "detail": "Found 4.5|-1.3|45 for field `result.cgpa`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 0 AND $value <= 4"
  }
]
```

#### query (different type)
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    result: {
        cgpa: hi|'hi'|[],
        ...,
    },
};
```
#### result
```json
[
  {
    "time": "740.7µs",
    "status": "ERR",
    "detail": "Found NONE|0|0 for field `result.cgpa`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 0 AND $value <= 4"
  }
]
```

### wrong input in `result.grade`
#### query
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    result: {
        ...,
        grade: string::trim('a+'|'cd'|'1234'|'f'),
    },
    
};
```
#### result
```json
[
  {
    "time": "710.8µs",
    "status": "ERR",
    "detail": "Found 'a+'|'cd'|'1234'|'f' for field `result.grade`, with record `student:2019331074`, but field must conform to: $value INSIDE ['A+', 'A', 'A-', 'B+', 'B', 'B-', 'C+', 'C', 'C-', 'F']"
  }
]
```

### wrong input in `result.total_credit`
#### query (type same but out of conditions)
```sql
CREATE student CONTENT {
    id : 2019331074,
    ...,
    result: {
        ...,
        total_credit: 163|-123,
    },
};
```
#### result
```json
[
  {
    "time": "788.8µs",
    "status": "ERR",
    "detail": "Found 163|-123 for field `result.total_credit`, with record `student:2019331074`, but field must conform to: $value != NONE AND $value >= 0 AND $value <= 160"
  }
]
```
#### query (different type)
## will be accepted as string counts as `0`
