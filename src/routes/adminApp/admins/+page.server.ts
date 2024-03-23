import { getAdminList } from "$lib/Database/admin";
import { getSimpleListTile } from "$lib/utils";

export async function load() {
    let admins = getSimpleListTile(await getAdminList());
    
    return {
        listItems: admins
    }
    
}