import { getStudentListByDepartment } from "$lib/Database/student";
import { getTeacherListByDepartment } from "$lib/Database/teacher";
import { getListTile } from "$lib/utils";

export async function load({locals}) {

    let students = getListTile(await getStudentListByDepartment(locals.user.department));
    let teachers = getListTile(await getTeacherListByDepartment(locals.user.department));
    return {
        students: students,
        teachers: teachers
    }
    
}