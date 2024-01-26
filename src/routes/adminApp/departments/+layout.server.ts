import { db } from "$lib/Database/surreal";
import type { ListTile } from "$lib/models.js";

export async function load() {

    let admins = await db.select('department');

    let listItems: ListTile[] = admins.map((admin: any) => {
        return {
            name: admin.name,
            id: admin.id
        }
    });
    
    return {
        pageName: 'Department',
        url: 'departments',
        listItems: listItems,
    }
    
}