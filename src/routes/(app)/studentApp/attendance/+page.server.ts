
import { getRegisteredCourses } from "$lib/Database/course";

export async function load({ locals }) {
    let courses = await getRegisteredCourses(locals.user.id);
    return {
        courses: courses
    }
    
}