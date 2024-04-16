import { getAssignment } from "$lib/Database/assignment.js";
import { getCourse } from "$lib/Database/course";

export async function load({ params }) {
    let assignment = await getAssignment(params.assignmentID);
    let course = await getCourse(params.courseID);
    return {
        details: assignment,
        course: course
    }
    
}