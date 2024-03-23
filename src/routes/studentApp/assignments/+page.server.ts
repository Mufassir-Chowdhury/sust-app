import { getAssignmentList } from "$lib/Database/assignment";
import { getListTile } from "$lib/utils";

export async function load() {
    let assignments = getListTile(await getAssignmentList());
    
    return {
        listItems: assignments
    }
    
}