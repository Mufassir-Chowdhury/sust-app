import { getCourseList } from "$lib/Database/course";
import { getListTile } from "$lib/utils";

export async function load({ locals }) {

    let courseList = getListTile(await getCourseList(locals.user.id));
    return {
        listItems: courseList
    }
    
}