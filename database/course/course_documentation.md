### To make a schemaful table `course` in database
```sql
DEFINE TABLE course SCHEMAFUL;
```

### To create a field `course_code` unique with
- type `string`
- lenght of `6 or 7 characters`
```sql
define field course_code on course type string
    value meta::id(id);
```

### To create a field `course_title` with
- type `string`
- accepting `only alpha` characters
- minimum length of `6`

```sql
define field course_title on course type string
    assert $value != none
    and $value = /^[A-Za-z ]+$/
    and string::len($value) > 5;
```

### To make the `course_title` field `unique`
```sql
define index course_title on course fields course_title unique;
```

### To create a field `credit` with
- type `float`
- between `0 to 3`

```sql
define field credit on course type float
    assert $value != none
    and $value > 0 and $value <= 3;
```

### To create a field `type` with
- type `string`
- fixed values
    - `Lab`
    - `Theory`
    - `Thesis`

```sql
define field type on course type string
    assert $value inside ['Lab', 'Theory', 'Thesis'];
```











2
2
2
2
2
2
2
2
2
2
