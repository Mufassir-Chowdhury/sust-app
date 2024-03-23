import { getNotice } from "$lib/Database/notice.js";

export async function load({ params }) {
    let exam = await getNotice(params.notice);
    return {
        details: exam
    }
    
}