import { getExam } from "$lib/Database/exam.js";

export async function load({ params }) {
    let exam = await getExam(params.exam);
    return {
        details: exam
    }
    
}