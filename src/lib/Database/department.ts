import { database, db } from "$lib/Database/surreal.js";



export async function getDepartmentName(id: string): Promise<any> {
    if(!database) return "Computer Science and Engineering";
    const result = await db.query<[[{name: string}]]>(
        'SELECT name from $dp',
        { "dp": id }
    );
    return result[0][0].name;
}