import { getCourse } from "$lib/Database/course";
import { getExam } from "$lib/Database/exam.js";

export async function load({ params }) {
    let exam = await getExam(params.examID);
    let course = await getCourse(params.courseID);
    return {
        details: exam,
        course: course
    }
    
}