import { getAssignmentList } from "$lib/Database/assignment";
import { getListTile } from "$lib/utils";

export async function load() {
    let admins = getListTile(await getAssignmentList());
    
    return {
        listItems: admins
    }
    
}