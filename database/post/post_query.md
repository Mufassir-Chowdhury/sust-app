- info - object
    - Title - string
    - Posted by - record:link - teacher
    - Date of post - datetime
- description - string
- pdf - url

```sql
DEFINE TABLE post SCHEMAFUL;

DEFINE FIELD info ON post TYPE object;

DEFINE FIELD info.title ON post TYPE string 
    ASSERT $value != NONE;

DEFINE FIELD info.posted_by ON post TYPE record(teacher) 
    ASSERT $value != NONE;

DEFINE FIELD info.date ON post TYPE datetime 
    ASSERT $value != NONE;

DEFINE FIELD description ON post TYPE string 
    ASSERT $value != NONE;

DEFINE FIELD pdf ON post TYPE string;
```

### Adding a post
```sql
relate course:CSE222->has->(select id from (create post content {
    description: 'this is the first post',
    pdf: '',
    info: {
        title: 'First Post',
        posted_by: teacher:2019331073,  
        date: time::now()
    }
})) content {
    year: '2023'
}
```

### fetching post
```sql
select *, info.posted_by.name as info.posted_by
from (select value ->(has where year='2023')->post as post 
from course:CSE222 
fetch teacher)
```