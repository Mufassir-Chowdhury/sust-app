import { getAdminList } from "$lib/Database/admin";
import { getListTile, getSimpleListTile } from "$lib/utils";

export async function load() {
    let admins = getSimpleListTile(await getAdminList());
    // let admins = getListTile(await getAdminList());
    return {
        listItems: admins
    }
    
}