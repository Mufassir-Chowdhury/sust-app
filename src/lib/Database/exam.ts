import { database, db } from "$lib/Database/surreal.js";

export async function getExamListByStudent(id: string) {
    if(database) return [
        {
            id: "exam:1",
            title: "exam 1",
            subtitle: "Introduction to Computer Science",
            status: "Not Submitted",
            status_color: "black",
            trailing: "2024-12-31",
        },
        {
            id: "exam:2",
            title: "exam 2",
            subtitle: "Introduction to Computer Science",
            status: "Submitted",
            status_color: "green",
            trailing: "2024-12-31",
        }
    ];
    const [record] = await db.query('SELECT <-has<-exam FROM $id', { id: id });
    return record;
}
export async function getExamListByCourse(id: string) {
    if(!database) return [
        {
            id: "exam:1",
            title: "exam 1",
            subtitle: "Introduction to Computer Science",
            status: "Not Submitted",
            status_color: "black",
            trailing: "2024-12-31",
        },
        {
            id: "exam:2",
            title: "exam 2",
            subtitle: "Introduction to Computer Science",
            status: "Submitted",
            status_color: "green",
            trailing: "2024-12-31",
        }
    ];
    const record = await db.query<[[exams: any]]>('SELECT ->has->exam as exams FROM $id FETCH exams', { id: id });
    return record[0][0].exams;
}

export async function getExam(id: string): Promise<any> {
    if(!database) return {
        id: "exam:1",
        title: "exam 1",
        creator: "Introduction to Computer Science",
        due_date: "2024-12-31",
        creation_date: "2024-12-01",
        total_grade: 100,
        status: "Not Submitted",
        status_color: "black",
        description: "This is the first exam of the course",
        attachments: [],
        submissions: true
    } ;
    const [record] = await db.select(id);
    return record;
}