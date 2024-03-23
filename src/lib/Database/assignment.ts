import { database, db } from "$lib/Database/surreal.js";

export async function getAssignmentList(){
    if(!database) return [
        {
            id: "assignment:1",
            title: "Assignment 1",
            subtitle: "Introduction to Computer Science",
            status: "Not Submitted",
            status_color: "black",
            trailing: "2024-12-31",
        },
        {
            id: "assignment:2",
            title: "Assignment 2",
            subtitle: "Introduction to Computer Science",
            status: "Submitted",
            status_color: "green",
            trailing: "2024-12-31",
        }
    ];
    // TODO connect with database
    return null;
}

export async function getAssignment(id: string): Promise<any> {
    if(!database) return {
        id: "assignment:1",
        title: "Assignment 1",
        course: "Introduction to Computer Science",
        submitted: false,
        due_date: "2024-12-31",
        creation_date: "2024-12-01",
        total_grade: 100,
        description: "This is the first assignment of the course",
        attachments: []
    } ;
    // TODO connect with database
    return null;
}