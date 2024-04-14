import { database, db } from "$lib/Database/surreal.js";
import type { Student } from "$lib/models";

export async function getStudentList(){
    if(!database) return [
        {
            id: "student:2019331073",
            title: "John Doe",
            department: "CSE",
        }
    ];
    return await db.select('student')
}

export async function getStudent(id: string): Promise<any> {
    if(!database) return {
        id: "student:2019331073",
        name: "John Doe",
        department: "CSE",
        current_semester: 3,
        session: "2019",
        email: {
            personal: "johndoe@mail.com",
            academic: "johndoe@sust.edu"
        },
        gender: "male",
        blood_group: "A+",
    } ;
    let [student] = await db.select<Record<string, Student>>(id);
    return student;
}