import { getAssignmentListByCourse } from "$lib/Database/assignment";
import { getClasses, getCourse, getInstructors } from "$lib/Database/course";
import { getDepartmentName } from "$lib/Database/department";
import { getExamListByCourse } from "$lib/Database/exam";
import { getStudentListByCourse } from "$lib/Database/student.js";
import { getListTile } from "$lib/utils";

export async function load({ params }) {
    let course = await getCourse(params.course);
    let departmentName = await getDepartmentName(course.department);
    let instructors = await getInstructors(params.course);
    let students = await getStudentListByCourse(params.course);
    let assignmentList = getListTile(await getAssignmentListByCourse(params.course));
    let examList = getListTile(await getExamListByCourse(params.course));
    let attendances = await getClasses(params.course);

    return {
        details: course,
        department: departmentName,
        instructors: instructors,
        students: students,
        assignments: assignmentList,
        exams: examList,
        attendances: attendances
    }
}