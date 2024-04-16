import { database, db } from "$lib/Database/surreal.js";

export async function getNoticeList(){
    if(!database) return [
        {
            title: "Computer Science and Engineering",
            id: "department:CSE",
        },
        {
            title: "Electrical and Electronic Engineering",
            id: "department:EEE",
        }
    ];
    return await db.select('notice')
}
export async function getNoticeListByDepartment(id: string){
    if(!database) return [
        {
            title: "Computer Science and Engineering",
            id: "department:CSE",
        },
        {
            title: "Electrical and Electronic Engineering",
            id: "department:EEE",
        }
    ];
    const [record] = await db.query('SELECT * FROM notice WHERE $id in department', {id: id});
    return record;
}


export async function getNotice(id: string): Promise<any> {
    if(!database) return {
        id: "notice:1",
        title: "Notice 1",
        creator: "Office of The Controller of Examinations",
        creation_date: "2024-12-01",
        status: "New",
        status_color: "red",
        description: "This is the first notice of the course",
        attachments: [],
        submissions: false
    } ;
    // TODO connect with database
    const [record] = await db.select(id);
    return record;
}