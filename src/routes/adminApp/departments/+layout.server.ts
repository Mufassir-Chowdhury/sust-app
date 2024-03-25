import { getDepartmentList } from "$lib/Database/department";
import { getSimpleListTile } from "$lib/utils";

export async function load() {

    let departments = getSimpleListTile(await getDepartmentList());
    
    return {
        listItems: departments,
    }
    
}