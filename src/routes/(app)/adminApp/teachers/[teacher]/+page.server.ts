import { getDepartmentName } from "$lib/Database/department.js";
import { getTeacher } from "$lib/Database/teacher";

export async function load({ params }) {
    let teacher = await getTeacher(params.teacher);
    const department = await getDepartmentName(teacher.department);
    return {
        details: teacher,
        department: department
    }
    
}