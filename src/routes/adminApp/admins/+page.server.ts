import type { ListTile } from "$lib/models.js";
import { getAdminList } from "$lib/Database/admin";
import { db } from "$lib/Database/surreal";

export async function load() {
    let admins = await getAdminList();

    let listItems: ListTile[] = admins.map((admin: any) => {
        return {
            name: admin.name,
            id: admin.id
        }
    });
    console.log(listItems)
    
    return {
        listItems: listItems
    }
    
}