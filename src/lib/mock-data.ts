import type { Attendance, Course, Student } from './models';

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
        },
    },
];

export const mockAttendance: Attendance[] = [
    {
        course_id: '1',
        date: '2023-04-01',
        students: {
            '2019331073': true,
        },
    },
];