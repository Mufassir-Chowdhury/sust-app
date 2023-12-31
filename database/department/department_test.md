### duplicate `code`
#### query
```sql
create department content {
    code : 331,
    id : string::trim('PHY'),
    letter_code : string::trim('PHY'),
    name : string::trim('Physics'),
    building : string::trim('A'),
    floor : 1,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "697.6µs",
    "status": "ERR",
    "detail": "Database index `code` already contains 331, with record `department:PHY`"
  }
]
```
### duplicate `id`

#### query
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
```
#### result
```json
[
  {
    "time": "94.8µs",
    "status": "ERR",
    "detail": "Database record `department:CSE` already exists"
  }
]
```
### duplicate `letter_code`

#### query
```sql
create department content {
    code : 123,
    id : string::trim('PHY'),
    letter_code : string::trim('CSE'),
    name : string::trim('Physics'),
    building : string::trim('A'),
    floor : 1,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "409.9µs",
    "status": "ERR",
    "detail": "Database index `letter_code` already contains 'CSE', with record `department:PHY`"
  }
]
```

### duplicate `minor_course_id`

#### query
```sql
create department content {
    code : 332,
    id : string::trim('CSR'),
    letter_code : string::trim('CSR'),
    name : string::trim('Computer cience and Engineering'),
    building : string::trim('Dr. M. A. Wazed Miah IICT Building'),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "821.8µs",
    "status": "ERR",
    "detail": "Database index `minor_course_code` already contains 'D', with record `department:CSR`"
  }
]
```

### duplicate `name`
#### query
```sql
create department content {
    code : 123,
    id : string::trim('PHY'),
    letter_code : string::trim('PHY'),
    name : string::trim('Computer Science and Engineering'),
    building : string::trim('A'),
    floor : 1,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "640.5µs",
    "status": "ERR",
    "detail": "Database index `name` already contains 'Computer Science and Engineering', with record `department:PHY`"
  }
]
```
### Wrong input in `code`
#### query (type same but out of conditions)
```sql
create department content {
    code : -123|1234|234.6|.990,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "316.4µs",
    "status": "ERR",
    "detail": "Found -123|1234|234.6|.990 for field `code`, with record `department:ECO`, but field must conform to: $value != NONE AND $value >= 100 AND $value <= 999 AND array::len(string::split(type::string($value), '.')) == 1"
  }
]
```
#### query (type diffrent)
```sql
create department content {
    code : true|hiii|'hiii'|false|['k','l']|'',
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "445µs",
    "status": "ERR",
    "detail": "Found 1|NONE|0|0|0|0 for field `code`, with record `department:ECO`, but field must conform to: $value != NONE AND $value >= 100 AND $value <= 999 AND array::len(string::split(type::string($value), '.')) == 1"
  }
]
```
### Wrong input in `letter_code`
#### query (type same but out of condition or different type)
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('EC67'|'EC'|'ECo'|'234'|'234.6'|''),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "306.3µs",
    "status": "ERR",
    "detail": "Found 'EC67'|'EC'|'ECo'|'234'|'234.6'|'' for field `letter_code`, with record `department:ECO`, but field must conform to: $value != NONE AND string::len($value) = 3 AND $value = /[A-Z]{3}/"
  }
]
```

### Wrong input in `minor_course_code`
#### query (type same but out of condition or different type)
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim(''|'    '|'aa'|'1'|'Ab'),
};
```
#### result
```json
[
  {
    "time": "321.8µs",
    "status": "OK",
    "result": []
  },
  {
    "time": "399.4µs",
    "status": "ERR",
    "detail": "Found ''|''|'aa'|'1'|'Ab' for field `minor_course_code`, with record `department:ECO`, but field must conform to: $value != NONE AND $value = /[A-Za-z]{1}/ AND string::len($value) == 1"
  }
]
```

### Wrong input in `name`
#### query
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('E'|'Economics 4'|'economics.first'|''|'     '|'    Ecomo6  '),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "487.9µs",
    "status": "ERR",
    "detail": "Found 'E'|'Economics 4'|'economics.first'|''|''|'Ecomo6' for field `name`, with record `department:ECO`, but field must conform to: $value != NONE AND $value = /^[A-Za-z ]+$/ AND string::len($value) > 5 AND array::len(string::words($value)) > 0"
  }
]
```
### Wrong input in `building`
#### query
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim(''|'     '),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "187.1µs",
    "status": "ERR",
    "detail": "Found ''|'' for field `building`, with record `department:ECO`, but field must conform to: $value != NONE AND string::len($value) > 0"
  }
]
```
### Wrong input in `floor`
#### query (type same but out of condition)
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 10.5|0|11|-1,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "444µs",
    "status": "ERR",
    "detail": "Found 10.5|0|11|-1 for field `floor`, with record `department:ECO`, but field must conform to: $value != NONE AND $value > 0 AND $value <= 10 AND array::len(string::split(type::string($value), '.')) == 1"
  }
]
```
#### query (type different)
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : hiii|'hiii'|false|['k','l']|'',
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "204µs",
    "status": "ERR",
    "detail": "Found NONE|0|0|0|0 for field `floor`, with record `department:ECO`, but field must conform to: $value != NONE AND $value > 0 AND $value <= 10 AND array::len(string::split(type::string($value), '.')) == 1"
  }
]
```
#### edge case
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : true,
    minor_course_code : string::trim('D'),
};
```
In this case data will be inserted and the value of `floor` will be 1. 



## Right input 
#### query
```sql
create department content {
    code : 100,
    id : string::trim('ECO'),
    letter_code : string::trim('ECO'),
    name : string::trim('Economics'),
    building : string::trim('D'),
    floor : 3,
    minor_course_code : string::trim('D'),
};
```
#### result
```json
[
  {
    "time": "1.026ms",
    "status": "OK",
    "result": [
      {
        "building": "D",
        "code": 100,
        "floor": 3,
        "id": "department:ECO",
        "letter_code": "ECO",
        "name": "Economics"
      }
    ]
  }
]
```