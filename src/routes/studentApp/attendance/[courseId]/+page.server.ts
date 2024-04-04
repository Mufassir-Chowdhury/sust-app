import { getAttendancesByCourseId, getCourseById, getStudentsByAttendanceId } from "$lib/Database/sajiddb";

export async function load({ params }) {
    const course: any = await getCourseById(params.courseId);
    const attendances: any = await getAttendancesByCourseId(params.courseId);
    const students: any = await getStudentsByAttendanceId(attendances[0].id);

    console.log(attendances)
    console.log("HER")
    return {
        course,
        attendances,
        students,
    };
}