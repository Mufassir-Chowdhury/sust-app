import { getClass, getCourse } from "$lib/Database/course.js";
import { getStudentListByCourse } from "$lib/Database/student.js";

export async function load( { params } ) {
    let course = await getCourse(params.courseId);
    let students = await getStudentListByCourse(params.courseId);
    let attendance = await getClass(params.attendanceId);
    return {
        course: course,
        students: students,
        attendance: attendance
    }
}