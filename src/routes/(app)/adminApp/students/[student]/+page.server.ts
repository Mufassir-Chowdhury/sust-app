import { getDepartmentName } from "$lib/Database/department.js";
import { getStudent } from "$lib/Database/student.js";

export async function load({ params }) {
    let student = await getStudent(params.student);
    const department = await getDepartmentName(student.department);
    return {
        details: student,
        department: department
    }
    
}