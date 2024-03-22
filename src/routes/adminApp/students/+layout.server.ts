import { getStudentList } from "$lib/Database/student";
import { getListTile } from "$lib/utils";

export async function load() {

    let students = getListTile(await getStudentList());
    
    return {
        pageName: 'Students',
        url: 'students',
        listItems: students
    }
    
}