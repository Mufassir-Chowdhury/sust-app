import { getAssignmentListByStudent } from "$lib/Database/assignment";
import { getListTile } from "$lib/utils";

export async function load({ locals }) {
    let assignments = getListTile(await getAssignmentListByStudent(locals.user.id));
    
    return {
        listItems: assignments
    }
    
}