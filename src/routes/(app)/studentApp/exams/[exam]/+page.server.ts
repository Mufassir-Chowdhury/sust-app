import { getCourseFromExam, getExam } from "$lib/Database/exam.js";

export async function load({ params }) {
    let exam = await getExam(params.exam);
    let course = await getCourseFromExam(params.exam);
    return {
        details: exam,
        course: course
    }
}