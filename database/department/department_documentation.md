### To make a schemafull table `department` in database
```sql
DEFINE TABLE department SCHEMAFULL;
```

### To create a field `code` with
- type `number`
- accepting only `3 digits integer number`
```sql
DEFINE FIELD code ON department TYPE number 
    ASSERT $value != NONE AND $value >= 100 AND $value <= 999 
    AND math::round($value) = $value;
```
### To make the `code` field `unique`
```sql
DEFINE INDEX code ON department FIELDS code UNIQUE;
```

### To create a field `letter_code` with
- type `string`
- exact length of `3 uppercase letters`
```sql
DEFINE FIELD letter_code ON department TYPE string 
    ASSERT $value != NONE AND string::len($value) = 3 AND $value = /[A-Z]{3}/;
```

### To make the `letter_code` field `unique`
```sql
DEFINE INDEX letter_code ON department FIELDS letter_code UNIQUE;
```

### To create a field `name` with
- type `string`
- acceptiong only `uppercase and lowercase letters and space`
- minimum length of `5`

```sql
DEFINE FIELD name ON department TYPE string 
    ASSERT $value != NONE AND $value = /^[A-Za-z ]+$/
    AND string::len($value) > 5;
```

### To make the `name` field `unique`
```sql
DEFINE INDEX name ON department FIELDS name UNIQUE;
```

### To create a field `minor_course_code` with
- type `string`
- `one character alphabet`

```sql
DEFINE FIELD minor_course_code ON department TYPE string 
    ASSERT $value != NONE AND $value = /[A-Z]{1}/
    AND string::len($value) == 1;
```

### To make the `minor_course_code` field `unique`
```sql
DEFINE INDEX minor_course_code ON department FIELDS minor_course_code UNIQUE;
```

### To create a field `building` with
- type `string`
- `non-empty` value
  
```sql
DEFINE FIELD building ON department TYPE string 
    ASSERT $value != NONE AND string::len($value) > 0;
```
### To create a field `floor` with
- type `number`
- accepting only `1 to 10 integer numbers` 

```sql
DEFINE FIELD floor ON department TYPE number 
    ASSERT $value != NONE AND $value > 0 AND $value <= 10 
    AND math::round($value) = $value;
```