import type { Attendance, Course, Student, CourseEnrollment } from './models';

export const mockCourses: Course[] = [
    {
        course_code: 'CSE101',
        credit: 3,
        department: 'CSE',
        id: '1',
        name: 'Introduction to Computer Science',
        syllabus: [],
        type: 'Theory',
    },
    {
        course_code: 'CSE102',
        credit: 3,
        department: 'CSE',
        id: '2',
        name: 'Programming Language',
        syllabus: [],
        type: 'Theory',
    },
];

export const mockStudents: Student[] = [
    {
        id: '2019331073',
        name: 'John Doe',
        department: 'CSE',
        current_semester: 3,
        session: 2019,
        email: {
            personal: 'johndoe@mail.com',
            academic: 'johndoe@sust.edu',
        },
        gender: 'male',
        blood_group: 'A+',
        personal: {
            birthday: '01/01/2000',
            father: 'John Doe Sr.',
            hometown: 'Sylhet',
            mother: 'Jane Doe',
            phone: '01711111111',
        },
        result: {
            cgpa: 3.5,
            grade: 'B+',
            total_credit: 60,
        }
    },
    // Add more mock students as needed
];

export const mockAttendance: Attendance[] = [
    {
        id: '1',
        course_id: '1',
        date: '2023-04-01',
        students: {
            '2019331073': true,
            // Add more student IDs with attendance status
        },
    },
    {
        id: '2',
        course_id: '2',
        date: '2023-04-02',
        students: {
            '2019331073': true,
            // Add more student IDs with attendance status
        },
    },
    // Add more mock attendance records as needed
];

export const mockCourseEnrollments: CourseEnrollment[] = [
    {
        "course_id": "1",
        "semester": "1/1",
        "session": "2019",
        "studentsIds": ["2019331073"]
    },
    {
        "course_id": "2",
        "semester": "1/2",
        "session": "2019",
        "studentsIds": ["2019331073"]
    }
];