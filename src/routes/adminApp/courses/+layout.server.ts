import { db } from "$lib/Database/surreal";
import type { ListTile } from "$lib/models.js";

export async function load() {

    let admins = await db.select('course');

    let listItems: ListTile[] = admins.map((admin: any) => {
        return {
            name: admin.name,
            id: admin.id
        }
    });
    
    return {
        pageName: 'Courses',
        url: 'courses',
        listItems: listItems
    }
    
}