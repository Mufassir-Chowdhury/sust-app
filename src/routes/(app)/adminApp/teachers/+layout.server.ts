import { getTeacherList } from "$lib/Database/teacher";
import { getListTile } from "$lib/utils";

export async function load() {

    let teachers = getListTile(await getTeacherList());
    
    return {
        listItems: teachers
    }
    
}