import { getClasses, getCourse } from "$lib/Database/course.js";

export async function load( { params } ) {
    let course = await getCourse(params.courseId);
    let attendances = await getClasses(params.courseId);
    return {
        course: course,
        attendances: attendances
    }
}