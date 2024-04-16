import { getNotice } from "$lib/Database/notice";

export async function load({ params }) {
    let notice = await getNotice(params.notice);
    return {
        details: notice
    }
}