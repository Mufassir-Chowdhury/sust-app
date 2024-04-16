import { getCourseList, getRegisteredCourses } from "$lib/Database/course";
import { getListTile } from "$lib/utils";

export async function load({ locals }) {

    let courseList = getListTile(await getRegisteredCourses(locals.user.id));
    return {
        listItems: courseList,
        user: locals.user
    }
    
}