import { getNoticeList } from "$lib/Database/notice";
import { getListTile } from "$lib/utils";

export async function load() {
    let exams = getListTile(await getNoticeList());
    
    return {
        listItems: exams
    }
    
}