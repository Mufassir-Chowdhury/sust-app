import { getDepartmentName } from "$lib/Database/department.js";
import { getStudent } from "$lib/Database/student.js";

export async function load({ params }) {
    let student = await getStudent(params.person);
    const department = await getDepartmentName(student.department);
    return {
        details: student,
        department: department
    }
    
}