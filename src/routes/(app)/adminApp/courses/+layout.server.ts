import { getCourseList } from "$lib/Database/course";
import { getListTile } from "$lib/utils";

export async function load() {

    let admins = getListTile(await getCourseList());
    
    return {
        listItems: admins
    }
    
}