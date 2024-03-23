import { getTeacherList } from "$lib/Database/teacher";
import { getSimpleListTile } from "$lib/utils";

export async function load() {

    let teachers = getSimpleListTile(await getTeacherList());
    
    return {
        pageName: 'Teachers',
        url: 'teachers',
        listItems: teachers
    }
    
}