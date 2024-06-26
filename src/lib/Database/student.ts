import { database, db } from "$lib/Database/surreal.js";
import type { Student } from "$lib/models";

export async function getStudentList(){
    if(!database) return [
        {
            id: "student:2019331073",
            name: "John Doe",
            department: "CSE",
        }
    ];
    return await db.select('student')
}
export async function getStudentListByDepartment(id: string){
    if(!database) return [
        {
            id: "student:2019331073",
            name: "John Doe",
            department: "CSE",
        }
    ];
    const [record] = await db.query<[[ any[] ]]>('SELECT * FROM student WHERE department=$id;', { "id": id });
    console.log(record);
    return record;
}
export async function getStudentListByCourse(id: string){
    if(!database) return [
        {
            id: "student:2019331073",
            name: "John Doe",
            department: "CSE",
        }
    ];
    return await db.query<[[{students: Student[]}]]>('SELECT <-takes<-student as students FROM $id FETCH students;', { "id": id }).then((res) => res[0][0].students);
}
export async function registerStudentToCourses(student: string, course: string[]){
    course.forEach(async (courseId) => {
        const result = await db.query<any>(
            'RELATE $student->takes->$course',
            { "student": student, "course": courseId }
        );
    });
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