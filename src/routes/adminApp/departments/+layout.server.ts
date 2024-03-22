import { getDepartmentList } from "$lib/Database/department";
import { getListTile } from "$lib/utils";

export async function load() {

    let departments = getListTile(await getDepartmentList());
    
    return {
        pageName: 'Department',
        url: 'departments',
        listItems: departments,
    }
    
}