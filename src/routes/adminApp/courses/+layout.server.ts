import { getCourseList } from "$lib/Database/course";
import { getSimpleListTile } from "$lib/utils";

export async function load() {

    let admins = getSimpleListTile(await getCourseList());
    
    return {
        listItems: admins
    }
    
}