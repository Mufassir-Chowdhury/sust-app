import { getExamList } from "$lib/Database/exam";
import { getListTile } from "$lib/utils";

export async function load() {
    let exams = getListTile(await getExamList());
    
    return {
        listItems: exams
    }
    
}