import { database, db } from "$lib/Database/surreal.js";
import type { Attendance, Course, Teacher } from "$lib/models";

export async function getCourseList(id: string | null = null){
    if(!database) return [
        {
            id: "course:CSE101",
            title: "Introduction to Computer Science",
            department: "CSE",
        },
        {
            id: "course:CSE102",
            title: "Programming Language",
            department: "CSE",
        }
    ];
    if(!id) return await db.select('course');
    return await db.query<[[{courses: Course[]}]]>('SELECT ->teaches->course as courses FROM $id FETCH courses;', { "id": id }).then((res) => res[0][0].courses);
}
export async function getRegisteredCourses(id: string | null = null){
    if(!database) return [
        {
            id: "course:CSE101",
            title: "Introduction to Computer Science",
            department: "CSE",
        },
        {
            id: "course:CSE102",
            title: "Programming Language",
            department: "CSE",
        }
    ];
    if(!id) return await db.select('course');
    return await db.query<[[{courses: Course[]}]]>('SELECT ->takes->course as courses FROM $id FETCH courses;', { "id": id }).then((res) => res[0][0].courses);
}
export async function getCourseByDepartment(id: string | null = null){
    if(!database) return [
        {
            id: "course:CSE101",
            title: "Introduction to Computer Science",
            department: "CSE",
        },
        {
            id: "course:CSE102",
            title: "Programming Language",
            department: "CSE",
        }
    ];
    if(!id) return await db.select('course');
    return await db.query<[any[]]>('SELECT * FROM course WHERE department=$id;', { "id": id }).then((res) => res[0]);
}

export async function getCourse(id: string): Promise<any> {
    if(!database) return {
        id: "course:CSE101",
        title: "Introduction to Computer Science",
        department: "CSE",
        type: "lab",
        credit: 3,
    } ;
    let [course] = await db.select<Record<string, Course>>(id);
    return course;
}
export async function getClasses(id: string){
    if(!database) return [
        {
            id: "class:1",
            course: "course:CSE101",
            date: "2021-10-10",
            type: "lab",
            attendance: [
                {
                    student: "student:2019331073",
                    status: "present"
                }
            ]
        }
    ];
    return await db.query<[[{classes: any[]}]]>('SELECT <-taken<-class as classes FROM $id FETCH classes;', { "id": id }).then((res) => res[0][0].classes);
}
export async function getClass(id: string) {
    if(!database) return {
        id: "class:1",
        course: "course:CSE101",
        date: "2021-10-10",
        attendance: [
            {
                student: "student:2019331073",
                status: true
            }
        ]
    };
    const [record] = await db.select<Record<string, any>>(id);
    return record;
}
export async function addClass(attendance: Attendance){
    if(!database) return;
    const [record] = await db.create('class', {
        course: attendance.course_id,
        date: attendance.date,
        attendance: attendance.students
    
    });
    await db.query('RELATE $id <-taken<- $record;', { "id": attendance.course_id, "record": record.id });
}

export async function getInstructors(id: string): Promise<any> {
    if(!database) return [
        {
            id: "instructor:1",
            name: "John Doe",
            department: "CSE",
        },
        {
            id: "instructor:2",
            name: "Jane Doe",
            department: "CSE",
        }
    ];
    const result = await db.query<[[{teachers: Teacher[]}]]>(
        'SELECT <-teaches<-teacher as teachers FROM $id FETCH teachers;',
        { "id": id }
    );
    return result[0][0].teachers;
}