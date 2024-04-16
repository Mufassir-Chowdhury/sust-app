import { getAssignmentListByCourse } from "$lib/Database/assignment.js";
import { getListTile } from "$lib/utils";

export async function load({ params }) {
    let assignmentList = getListTile(await getAssignmentListByCourse(params.courseID));
    return {
        listItems: assignmentList
    }
    
}