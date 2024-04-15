import { getCourseById, getAttendancesByCourseId } from "$lib/Database/sajiddb";

export async function load( { params } ) {
    let course = await getCourseById(params.courseId);
    let attendances = await getAttendancesByCourseId(params.courseId);

    return {
        course: course,
        attendances: attendances
    }
}