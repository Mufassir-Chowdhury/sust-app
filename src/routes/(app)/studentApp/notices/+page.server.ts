import { getNoticeListByDepartment } from "$lib/Database/notice";
import { getListTile } from "$lib/utils";

export async function load({ locals }) {
    let notices = getListTile(await getNoticeListByDepartment(locals.user.department));
    
    return {
        listItems: notices
    }
    
}