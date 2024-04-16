import { getExamListByStudent } from "$lib/Database/exam";
import { getListTile } from "$lib/utils";

export async function load({ locals }) {
    let exams = getListTile(await getExamListByStudent(locals.user.id));
    return {
        listItems: exams
    }
    
}