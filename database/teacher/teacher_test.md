### duplicate `id`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Database record `teacher:2019331073` already exists"
  }
]
```

### duplicate `email.personal`

#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Database index `personal_email` already contains 'mac22214u@gmail.com', with record `teacher:2019331074`"
  }
]
```

### duplicate `email.academic`

#### query
```sql
CREATE teacher CONTENT {
    id : 2019331074,
    email: {
       ...,
       academic:string::trim( 'mufassir73@sust.edu'),
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
    "detail": "Database index `academic_email` already contains 'mufassir73@sust.edu', with record `teacher:2019331074`"
  }
]
```

### duplicate `personal.phone`

#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Database index `phone` already contains 1771144308, with record `teacher:2019331074`"
  }
]
```

### wrong input in `name`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found 'Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y' for field `name`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3"
  }
]
```

### `department` will be detect automatically

### wrong input in `designation`
#### query
```sql

CREATE teacher CONTENT {
    id : 2019331074,
    designation : string::trim('Lectrer'|'1234'|'Asistant'),
    ...,
};
```
#### result
```json
[
  {
    "time": "245.4µs",
    "status": "ERR",
    "detail": "Found 'Lectrer'|'1234'|'Asistant' for field `designation`, with record `teacher:2019331074`, but field must conform to: $value INSIDE ['Lecturer', 'Assistant Professor', 'Professor']"
  }
]
```

### wrong input in `email.personal`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found 'mac22214gmail.com'|''|'1234'|'@gmail.com' for field `email.personal`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND is::email($value)"
  }
]
```

### wrong input in `email.academic`
#### query
```sql
CREATE teacher CONTENT {
    id : 2019331074,
    email: {
       academic:string::trim( 'mufassir74@sut.edu'|''|'1234'|'asanul@student.sust.edu'|'13@sust.edu'),
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
    "detail": "Found 'mufassir74@sut.edu'|''|'1234'|'asanul@student.sust.edu'|'13@student.sust.edu' for field `email.academic`, with record `student:2019331074`, but field must conform to: $value != NONE AND is::email($value) AND parse::email::host($value) = 'sust.edu' AND parse::email::user($value) >= 3"
  }
]
```

### wrong input in `gender`
#### query
```sql
CREATE teacher CONTENT {
    id : 2019331074,
    gender: string::lowercase('mle'|''|'1234'),
    ...,
};
```
#### result
```json
[
  {
    "time": "779.4µs",
    "status": "ERR",
    "detail": "Found 'mle'|''|'1234' for field `gender`, with record `teacher:2019331074`, but field must conform to: $value INSIDE ['male', 'female']"
  }
]
```

### wrong input in `blood_group`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found 'B'|'123'|'b+' for field `blood_group`, with record `teacher:2019331074`, but field must conform to: $value INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-']"
  }
]
```

### wrong input in `personal.father`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found 'Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y' for field `personal.father`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3"
  }
]
```

### wrong input in `personal.mother`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found 'Mufassir Ahmad Chowdhury4'|''|'1234'|'mr. x, y' for field `personal.mother`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z. ]+$/ AND string::len($value) >= 3"
  }
]
```

### wrong input in `personal.birthday`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found '2023-04-24T16:20:27.412492Z' for field `personal.birthday`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND time::year($value) < time::year() - 15"
  }
]
```

### wrong input in `personal.phone`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found 771144309|2774782347 for field `personal.phone`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND math::round($value) = $value AND string::startsWith(<string> $value, '1') AND string::len(<string> $value) = 10"
  }
]
```

### wrong input in `personal.hometown`
#### query
```sql
CREATE teacher CONTENT {
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
    "detail": "Found ''|'hi'|'rajshahi4'|'sylhet.'|'1234' for field `personal.hometown`, with record `teacher:2019331074`, but field must conform to: $value != NONE AND $value = /^[A-Za-z, ]+$/ AND string::len($value) >= 3"
  }
]
```

