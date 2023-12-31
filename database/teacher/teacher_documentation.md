### To make a schemafull table `teacher`
```sql
DEFINE TABLE teacher SCHEMAFULL
```

### To create a field `name` with
- type `string`
- accepting `only alpha character`
- containing `.` and `space` as `additional characters`
- at least length of `3`

```sql
DEFINE FIELD name ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z .]+$
    AND string::len($value) >= 3
```

### To create a field `department` with
- type `record link`
```sql
DEFINE FIELD department ON teacher TYPE record(department) 
    ASSERT $value != NONE
```

### To create a field `designation` with
- type `string`
- fixed values
    - Lecturer
    - Assistant Professor
    - Professor

```sql
DEFINE FIELD designation ON teacher TYPE string 
    ASSERT $value 
    INSIDE ['Lecturer', 'Assistant Professor', 'Professor']
```

### To create a field `email` with
- type `object`
```sql
DEFINE FIELD email ON teacher TYPE object
```

### To create a field `email.personal` with
- type `email`

```sql
DEFINE FIELD email.personal ON teacher TYPE string 
    ASSERT $value != NONE 
    AND is::email($value)
```

### To make the `email.personal` field `unique`

```sql
DEFINE INDEX personal_email ON teacher FIELDS email.personal UNIQUE
```

### To create a field `email.academic` with
- type `email`
- host name `sust.edu`
- user name of length `at least 3`
  
```sql
DEFINE FIELD email.academic ON teacher TYPE string 
    ASSERT $value != NONE 
    AND is::email($value) 
    AND parse::email::host($value) = 'sust.edu' 
    AND parse::email::user($value) >= 3
```

### To make `email.academic` field `unique` 

```sql
DEFINE INDEX academic_email ON teacher FIELDS email.academic UNIQUE
```

### To make a field `gender` with
- type `string`
- fixed values 
  - `male`
  - `female`

```sql
DEFINE FIELD gender ON teacher TYPE string 
    ASSERT $value INSIDE ['male', 'female']
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
DEFINE FIELD blood_group ON teacher TYPE string 
    ASSERT $value 
    INSIDE ['A+', 'B+', 'AB+', 'O+', 'A-', 'B-', 'AB-', 'O-']
```

### To create a field `personal` with
- type `object`

```sql
DEFINE FIELD personal ON teacher TYPE object
```

### To create a field `personal.father` name with
- type `string`
- accepting `only alpha character`
- containing `.` and `space` as `additional characters`
- at least length of `3`

```sql
DEFINE FIELD personal.father ON teacher TYPE string 
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
DEFINE FIELD personal.mother ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z. ]+$/ 
    AND string::len($value) >= 3
```

### To create a field `personal.birthday` with
-type `datetime`
- smaller than recent year by `15 years`

```sql
DEFINE FIELD personal.birthday ON teacher TYPE datetime 
    ASSERT $value != NONE 
    AND time::year($value) < time::year() - 15
```

### To crate a field `personal.phone` number with
- type `int`
- length of `10 digits` 
- starts with `1`

```sql
DEFINE FIELD personal.phone ON teacher TYPE number 
    ASSERT $value != NONE 
    AND math::round($value) = $value
    AND string::len(<string> $value) = 10
    AND string::startsWith(<string> $value, '1') 
```

### To make `personal.phone` field `unique`

```sql
DEFINE INDEX phone ON teacher FIELDS personal.phone UNIQUE
```

### To create a field `personal.hometown` with
- type `string`
- accepting `only alpha character`
- containing `,` and `space` as `additional characters`
- at least length of `3`

```sql
DEFINE FIELD personal.hometown ON teacher TYPE string 
    ASSERT $value != NONE 
    AND $value = /^[A-Za-z, ]+$/ 
    AND string::len($value) >= 3
```

