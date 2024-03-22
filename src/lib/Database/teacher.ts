import { database, db } from "$lib/Database/surreal.js";
import type { Course } from "$lib/models";

export async function getCourseList(){
    if(!database) return [
        {
            id: "course:CSE101",
            name: "Introduction to Computer Science",
            department: "CSE",
        },
        {
            id: "course:CSE102",
            name: "Programming Language",
            department: "CSE",
        }
    ];
    return await db.select('course')
}

export async function getCourse(id: string): Promise<any> {
    if(!database) return {
        id: "course:CSE101",
        name: "Introduction to Computer Science",
        department: "CSE",
        type: "lab",
        credit: 3,
    } ;
    let [course] = await db.select<Record<string, Course>>(id);
    return course;
}