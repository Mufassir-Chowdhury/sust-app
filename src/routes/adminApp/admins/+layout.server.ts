import type { ListTile } from "$lib/models.js";
import { db } from "$lib/Database/surreal.js";

export async function load() {
    // Get the list of admins from the database
    let admins = await db.select('admin');

    let listItems: ListTile[] = admins.map((admin: any) => {
        return {
            name: admin.name,
            id: admin.id
        }
    });
    
    return {
        pageName: 'Admins',
        url: 'admins',
        listItems: listItems
    }
    
}