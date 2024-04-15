import { database, db } from "$lib/Database/surreal";
import type { Admin } from "$lib/models";

export async function getAdminList(){
    if(!database) return [
        {
            title: "Admin",
            id: "admin:2019331004",
            department: "CSE",
        }
    ];
    const admins = await db.query('SELECT * from admin');
    console.log(admins)
    return await db.select('admin');
}

export async function getAdmin(id: string): Promise<any> {
    if(!database) return {
        id: id,
        name: "Admin",
        department: "CSE",
        email: {
            academic: "mrx@sust.edu",
            personal: "admin@google.com"
        },
        designation: "Assistant Something",
        gender: "male",
        blood_group: "A+"
    } ;
    let [admin] = await db.select<Record<string, Admin>>(id);
    return admin;
}