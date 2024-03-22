import { getAdminList } from "$lib/Database/admin";
import { getListTile } from "$lib/utils";

export async function load() {
    let admins = getListTile(await getAdminList());
    
    return {
        listItems: admins
    }
    
}