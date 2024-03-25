import { getStudentList } from "$lib/Database/student";
import { getSimpleListTile } from "$lib/utils";

export async function load() {

    let students = getSimpleListTile(await getStudentList());
    
    return {
        listItems: students
    }
    
}