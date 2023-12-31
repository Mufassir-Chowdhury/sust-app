### To make a schemafull table `student` in databse
```sql
DEFINE TABLE student SCHEMAFULL;
```

### To create a field `name` with
- type `string`
- accepting `only alpha character`
- containing `.` and `space` as `additional characters`
- at least length of `3`
```sql
DEFINE FIELD name ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3
```

### To create a field `department` with
- type `record link`
```sql
DEFINE FIELD department ON student TYPE record(department) 
    ASSERT $value != NONE
```

### To create a field `email` with
- type `object`
```sql
DEFINE FIELD email ON student TYPE object
```

### To create a field `email.personal` with
- type `email`

```sql
DEFINE FIELD email.personal ON student TYPE string 
    ASSERT $value != NONE 
    AND is::email($value)
```

### To make the `email.personal` field `unique`

```sql
DEFINE INDEX personal_email ON student FIELDS email.personal UNIQUE
```

### To create a field `email.academic` with
- type `email`
- host name `student.sust.edu`
- user name of length `5 at least`
- `last 2 characters` of user name contains only number
- user name `except the last 2 characters` contains only `alpha` characters.

```sql
DEFINE FIELD email.academic ON student TYPE string 
    ASSERT $value != NONE 
    AND is::email($value) 
    AND parse::email::host($value) = 'student.sust.edu' 
    AND parse::email::user($value) >= 5
    AND is::numeric(string::slice(parse::email::user($value), string::len(parse::email::user($value)) - 2, 2)) 
    AND is::alpha(string::slice(parse::email::user($value), 0, string::len(parse::email::user($value)) - 2))
```

### To make `email.academic` field `unique` 

```sql
DEFINE INDEX academic_email ON student FIELDS email.academic UNIQUE
```


### To make a field `gender` with
- type `string`
- fixed values 
  - `male`
  - `female`

```sql
DEFINE FIELD gender ON student TYPE string 
    ASSERT $value INSIDE ['male', 'female']
```


### To make a field `session` with
- type `integer`
- length of `4 digits`
- between `1980 to recent year`

```sql
DEFINE FIELD session ON student TYPE number 
    ASSERT $value != NONE 
    AND math::round($value) = $value
    AND $value >= 1980 AND $value <= time::year() 
```


### To make a field `current_semester` with
- type `int`
- accepting values between `1 to 8` only

```sql
DEFINE FIELD current_semester ON student TYPE number 
    ASSERT $value != NONE 
    AND math::round($value) = $value
    AND $value >= 1 AND $value <= 8 
```

### To make a field `blood_group` with
- type `string`
- fixed values
   - `A+`
   - `A-`
   - `B+`
   - `B-`
   - `AB+`
   - `AB-`
   - `O+`
   - `O-`

```sql
DEFINE FIELD blood_group ON student TYPE string 
    ASSERT $value 
    INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-']
```

### To make a field `privilege` with
- type `string`
- can be `NONE`
- `or` accepting only `alpha characters` and `.` and `(space)` as `additional` characters.

```sql
DEFINE FIELD privilege ON student TYPE string 
    ASSERT $value = NONE 
    OR ($value = /^[A-Za-z. ]+$/ 
        AND string::len($value) > 1)
```

### To create a field `personal` with
- type `object`

```sql
DEFINE FIELD personal ON student TYPE object
```

### To create a field `personal.father` name with
- type `string`
- accepting `only alpha character`
- containing `.` and `space` as `additional characters`
- at least length of `3`

```sql
DEFINE FIELD personal.father ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3
```

### To create a field `personal.mother` name with
- type `string`
- accepting `only alpha character`
- containing `.` and `space` as `additional characters`
- at least length of `3`

```sql
DEFINE FIELD personal.mother ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3
```

### To create a field `personal.birthday` with
-type `datetime`
- smaller than recent year by `15 years`

```sql
DEFINE FIELD personal.birthday ON student TYPE datetime 
    ASSERT $value != NONE 
    AND time::year($value) < time::year() - 15
```

### To crate a field `personal.phone` number with
- type `int`
- length of `10 digits` 
- starts with `1`

```sql
DEFINE FIELD personal.phone ON student TYPE number 
    ASSERT $value != NONE 
    AND math::round($value) = $value
    AND string::len(<string> $value) = 10
    AND string::startsWith(<string> $value, '1') 
```

### To make `personal.phone` field `unique`

```sql
DEFINE INDEX phone ON student FIELDS personal.phone UNIQUE
```

### To create a field `personal.hometown` with
- type `string`
- accepting `only alpha character`
- containing `,` and `space` as `additional characters`
- at least length of `3`

```sql
DEFINE FIELD personal.hometown ON student TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3
```

### To make a field `result` with
- type `object`

```sql
DEFINE FIELD result ON student TYPE object
```

### To make a field `result.cgpa` with
- type `float`
- between `0 to 4`

```sql
DEFINE FIELD result.cgpa ON student TYPE float 
    ASSERT $value != NONE 
    AND $value >= 0 AND $value <= 4
```

### To make a field `result.grade` with
- type `string`
- fixed values
    - `A+`
    - `A`
    - `A-`
    - `B+`
    - `B`
    - `B-`
    - `C+`
    - `C`
    - `C-`
    - `F`

```sql
DEFINE FIELD result.grade ON student TYPE string 
    ASSERT $value 
    INSIDE ['A+', 'A', 'A-', 'B+', 'B', 'B-', 'C+', 'C', 'C-', 'F']
```

### To make a field `result.total_credit` with
- type `float`
- between `0 to 160`

```sql
DEFINE FIELD result.total_credit ON student TYPE float 
    ASSERT $value != NONE 
    AND $value >= 0 AND $value <= 160
```

