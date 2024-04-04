export class NavItem {

    constructor(name: string, url: string, icon: string) {
        this.name = name;
        this.url = url;
        this.icon = icon;
    }
    name: string;
    url: string;
    icon: string;
}
export class Group {
    constructor(title: string, pages: string[]) {
        this.title = title;
        this.pages = pages;
    }
    title: string;
    pages: string[];
}

export class ListTile{
    title: string;
    subtitle: string;
    status: string | undefined;
    trailing: string | undefined;
    status_color: string | undefined;
    id: string;
    constructor(title: string, subtitle: string, status: string, trailing: string, id: string, status_color: string){
        this.title = title;
        this.subtitle = subtitle;
        this.status = status;
        this.trailing = trailing;
        this.status_color = status_color;
        this.id = id;
    }
}

export class Option{
    value: string;
    name: string;
    constructor(value: string, name: string){
        this.value = value;
        this.name = name;
    }
}

export class Email {
    academic: string;
    personal: string;
    constructor(academic: string, personal: string){
        this.academic = academic;
        this.personal = personal;
    }
}
export class Personal{
    birthday: string;
    father: string;
    hometown: string;
    mother: string;
    phone: string;
    constructor(birthday: string, father: string, hometown: string, mother: string, phone: string){
        this.birthday = birthday;
        this.father = father;
        this.hometown = hometown;
        this.mother = mother;
        this.phone = phone;
    }
}

class Person{
    blood_group: string;
    department: string;
    gender: string;
    id: string;
    name: string;
    email: Email;
    personal: Personal;
    constructor(blood_group: string, department: string, gender: string, id: string, name: string, email: Email, personal: Personal){
        this.blood_group = blood_group;
        this.department = department;
        this.gender = gender;
        this.id = id;
        this.name = name;
        this.email = email;
        this.personal = personal;
    }
}
export class Admin extends Person{
    designation: string;
    constructor(blood_group: string, department: string, designation: string, gender: string, id: string, name: string, email: Email, personal: Personal){
        super(blood_group, department, gender, id, name, email, personal);
        this.blood_group = blood_group;
        this.department = department;
        this.designation = designation;
        this.gender = gender;
        this.id = id;
        this.name = name;
        this.email = email;
        this.personal = personal;
    }
}

export class Teacher extends Person{
    designation: string;
    constructor(blood_group: string, department: string, designation: string, gender: string, id: string, name: string, email: Email, personal: Personal){
        super(blood_group, department, gender, id, name, email, personal);
        this.blood_group = blood_group;
        this.department = department;
        this.designation = designation;
        this.gender = gender;
        this.id = id;
        this.name = name;
        this.email = email;
        this.personal = personal;
    }
}


export class Result{
    cgpa: number;
    grade: string;
    total_credit: number;
    constructor(cgpa: number, grade: string, total_credit: number){
        this.cgpa = cgpa;
        this.grade = grade;
        this.total_credit = total_credit;
    }
}

export class Student extends Person{
    current_semester: number;
    result: Result;
    session: number;
    constructor(blood_group: string, current_semester: number, department: string, email: Email, gender: string, id: string, name: string, personal: Personal, result: Result, session: number){
        super(blood_group, department, gender, id, name, email, personal);
        this.blood_group = blood_group;
        this.current_semester = current_semester;
        this.department = department;
        this.email = email;
        this.gender = gender;
        this.id = id;
        this.name = name;
        this.personal = personal;
        this.result = result;
        this.session = session;
    }
}

export class Syllabus{
    title: string;
    topics: string[];
    constructor(title: string, topics: string[]){
        this.title = title;
        this.topics = topics;
    }
}

export class Course{
    course_code: string;
    credit: number;
    department: string;
    id: string;
    name: string;
    syllabus: Syllabus[];
    type: string;
    constructor(course_code: string, credit: number, department: string, id: string, name: string, syllabus: Syllabus[], type: string){
        this.course_code = course_code;
        this.credit = credit;
        this.department = department;
        this.id = id;
        this.name = name;
        this.syllabus = syllabus;
        this.type = type;
    }
}

export class Department{
    building: string;
    code: number;
    floor: number;
    id: string;
    letter_code: string;
    minor_course_code: string;
    name: string;
    constructor(building: string, code: number, floor: number, id: string, letter_code: string, minor_course_code: string, name: string){
        this.building = building;
        this.code = code;
        this.floor = floor;
        this.id = id;
        this.letter_code = letter_code;
        this.minor_course_code = minor_course_code;
        this.name = name;
    }
}

export class Attendance {
    id: string;
    course_id: string;
    date: string;
    students: { [studentId: string]: boolean };

    constructor(id: string, course_id: string, date: string, students: { [studentId: string]: boolean }) {
        this.id = id;
        this.course_id = course_id;
        this.date = date;
        this.students = students;
    }
}

export class CourseEnrollment {
    course_id: string;
    semester: string;
    session: string;
    studentsIds: string[];

    constructor(course_id: string, semester: string, sessions: string, studentsIds: string[]) {
        this.course_id = course_id;
        this.studentsIds = studentsIds;
        this.semester = semester;
        this.session = sessions;
    }
}
