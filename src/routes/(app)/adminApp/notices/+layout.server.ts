import { getDepartmentList } from "$lib/Database/department";
import { getNoticeList } from "$lib/Database/notice";
import { getListTile } from "$lib/utils";

export async function load() {

    let notices = getListTile(await getNoticeList());
    let departments = getListTile(await getDepartmentList());

    return {
        listItems: notices,
        departments: departments
    }
    
}