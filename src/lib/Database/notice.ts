import { database, db } from "$lib/Database/surreal.js";

export async function getNoticeList(){
    if(!database) return [
        {
            id: "notice:1",
            title: "Notice 1",
            subtitle: "Office of The Controller of Examinations",
            status: "New",
            status_color: "red",
            trailing: "2024-12-31",
        },
        {
            id: "notice:2",
            title: "Notice 2",
            subtitle: "Office of the Registrar",
            status: "Seen",
            status_color: "black",
            trailing: "2024-12-31",
        }
    ];
    // TODO connect with database
    return null;
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
    return null;
}