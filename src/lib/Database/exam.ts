import { database, db } from "$lib/Database/surreal.js";

export async function getExamList(){
    if(!database) return [
        {
            id: "exam:1",
            title: "Exam 1",
            subtitle: "Introduction to Computer Science",
            trailing: "2024-12-31",
            status: "Not Submitted",
            status_color: "black",
        },
        {
            id: "exam:2",
            title: "Exam 2",
            subtitle: "Introduction to Computer Science",
            trailing: "2024-12-31",
            status: "Not Submitted",
            status_color: "black",
        }
    ];
    // TODO connect with database
    return null;
}

export async function getExam(id: string): Promise<any> {
    if(!database) return {
        id: "exam:1",
        title: "Exam 1",
        course: "Introduction to Computer Science",
        due_date: "2024-12-31",
        creation_date: "2024-12-01",
        total_grade: 100,
        status: "Not Submitted",
        status_color: "black",
        description: "This is the first exam of the course",
        attachments: [],
        submissions: false
    } ;
    // TODO connect with database
    return null;
}