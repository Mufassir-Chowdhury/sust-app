import { getCourseList } from "$lib/Database/course";

export async function load({ locals }) {
    let courses = await getCourseList(locals.user.id);
    return {
        courses: courses
    }
    
}