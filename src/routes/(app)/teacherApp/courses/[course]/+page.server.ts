import { getCourse, getInstructors } from "$lib/Database/course";
import { getDepartmentName } from "$lib/Database/department";

export async function load({ params }) {
    let course = await getCourse(params.course);
    let departmentName = await getDepartmentName(course.department);
    let instructors = await getInstructors(params.course);
    return {
        details: course,
        department: departmentName,
        instructors: instructors
    }
}