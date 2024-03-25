import { getCourseById, getStudentsByAttendanceId, getAttendanceById } from "$lib/Database/sajiddb";

export async function load( { params } ) {
    let course = await getCourseById(params.courseId);
    let students = await getStudentsByAttendanceId(params.courseId);
    let attendance = await getAttendanceById(params.attendanceId);
    return {
        course: course,
        students: students,
        attendance: attendance
    }
}