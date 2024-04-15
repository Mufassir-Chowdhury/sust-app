import { getCourses } from "$lib/Database/sajiddb";

export async function load() {
    let courses = await getCourses();
    return {
        courses: courses
    }
    
}