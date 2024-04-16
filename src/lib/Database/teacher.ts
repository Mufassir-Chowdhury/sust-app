import { database, db } from "$lib/Database/surreal.js";
import type { Teacher } from "$lib/models";

export async function getTeacherList(){
    if(!database) return [
        {
            id: "teacher:1",
            title: "John Doe",
            department: "CSE",
        }
    ];
    return await db.select('teacher')
}
export async function getTeacherListByDepartment(id: string){
    if(!database) return [
        {
            id: "student:2019331073",
            name: "John Doe",
            department: "CSE",
        }
    ];
    const [record] = await db.query<[[ any[] ]]>('SELECT * FROM teacher WHERE department=$id;', { "id": id });
    console.log(record);
    return record;
}
export async function assignTeacherToCourse(teacher: string, course: string){
    const result = await db.query<any>(
        'RELATE $teacher->teaches->$course',
        { "teacher": teacher, "course": course }
    );
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