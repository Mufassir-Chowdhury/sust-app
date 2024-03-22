import { database, db } from "$lib/Database/surreal.js";
import type { Teacher } from "$lib/models";

export async function getTeacherList(){
    if(!database) return [
        {
            id: "teacher:1",
            name: "John Doe",
            department: "CSE",
        }
    ];
    return await db.select('teacher')
}

export async function getTeacher(id: string): Promise<any> {
    if(!database) return {
        id: "teacher:1",
        name: "John Doe",
        department: "CSE",
        email: {
            personal: "johndoe@mail.com",
            academic: "johndoe@sust.edu"
        },
        gender: "male", 
        designation: "Assistant Professor",
        blood_group: "A+",
    } ;
    let [teacher] = await db.select<Record<string, Teacher>>(id);
    return teacher;
}