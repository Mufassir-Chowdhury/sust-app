import { getStudentList } from "$lib/Database/student";
import { getSimpleListTile } from "$lib/utils";

export async function load() {

    let students = getSimpleListTile(await getStudentList());
    
    return {
        pageName: 'Students',
        url: 'students',
        listItems: students
    }
    
}