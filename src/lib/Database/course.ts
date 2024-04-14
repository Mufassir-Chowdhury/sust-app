import { database, db } from "$lib/Database/surreal.js";
import type { Course, Teacher } from "$lib/models";

export async function getCourseList(){
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
    return await db.select('course')
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