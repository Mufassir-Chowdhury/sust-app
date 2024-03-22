import { getCourse } from "$lib/Database/course";
import { getDepartmentName } from "$lib/Database/department";

export async function load({ params }) {
    let course = await getCourse(params.course);
    let departmentName = await getDepartmentName(course.department);
    return {
        details: course,
        department: departmentName
    }
}