import requests

reqUrl = "http://localhost:8000/sql"

headersList = {
 "Accept": "application/json",
 "NS": "test",
 "DB": "test",
 "Authorization": "Basic cm9vdDpyb290",
 "Content-Type": "text/plain" 
}

payload = "\
    DEFINE TABLE department SCHEMAFULL;\
    DEFINE FIELD code ON department TYPE number\
        ASSERT $value != NONE AND $value >= 100 AND $value <= 999 \
        AND math::round($value) = $value;\
    DEFINE INDEX code ON department FIELDS code UNIQUE;\
    DEFINE FIELD letter_code ON department TYPE string \
        ASSERT $value != NONE AND string::len($value) = 3 AND $value = /[A-Z]{3}/;\
    DEFINE INDEX letter_code ON department FIELDS letter_code UNIQUE;\
    DEFINE FIELD name ON department TYPE string \
        ASSERT $value != NONE AND $value = /^[A-Za-z ]+$/\
        AND string::len($value) > 5;\
    DEFINE INDEX name ON department FIELDS name UNIQUE;\
    DEFINE FIELD building ON department TYPE string \
        ASSERT $value != NONE AND string::len($value) > 0;\
    DEFINE FIELD floor ON department TYPE number \
        ASSERT $value != NONE AND $value > 0 AND $value <= 10 AND \
        math::round($value) = $value;\
    DEFINE FIELD minor_course_code ON department TYPE string \
        ASSERT $value != NONE AND $value = /[A-Za-z]{1}/ \
        AND string::len($value) == 1;\
create department content {\
    code : 331,\
    id : string::trim('CSE'),\
    letter_code : string::trim('CSE'),\
    name : string::trim('Computer Science and Engineering'),\
    building : string::trim('Dr. M. A. Wazed Miah IICT Building'),\
    floor : 3,\
    minor_course_code : string::trim('D'),\
};\
\
create department content {\
    code : 123,\
    id : string::trim('SWE'),\
    letter_code : string::trim('SWE'),\
    name : string::trim('Software Engineering'),\
    building : string::trim('Dr. M. A. Wazed Miah IICT Building'),\
    floor : 1,\
    minor_course_code : string::trim('A'),\
};\
\
create department content {\
    code : 330,\
    id : string::trim('ECO'),\
    letter_code : string::trim('ECO'),\
    name : string::trim('Economics'),\
    building : string::trim('D'),\
    floor : 3,\
    minor_course_code : string::trim('B'),\
};\
\
create department content {\
    code : 132,\
    id : string::trim('PHY'),\
    letter_code : string::trim('PHY'),\
    name : string::trim('Physics'),\
    building : string::trim('A'),\
    floor : 1,\
    minor_course_code : string::trim('a'),\
};"

response = requests.request("POST", reqUrl, data=payload,  headers=headersList)

print(response.text)