import { getExamListByCourse } from "$lib/Database/exam";
import { getListTile } from "$lib/utils";

export async function load({ params }) {
    let examList = getListTile(await getExamListByCourse(params.courseID));
    return {
        listItems: examList
    }
    
}