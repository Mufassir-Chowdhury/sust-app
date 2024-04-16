import { getClasses, getCourse } from "$lib/Database/course.js";

export async function load({ params, locals }) {
    const course = await getCourse(params.courseId);
    const attendances = await getClasses(params.courseId);
    return {
        course,
        attendances,
        user: locals.user.id
    };
}