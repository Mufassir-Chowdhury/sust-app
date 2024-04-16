import { getAssignment, getCourseFromAssignment } from "$lib/Database/assignment.js";

export async function load({ params }) {
    let assignment = await getAssignment(params.assignment);
    let course = await getCourseFromAssignment(params.assignment);
    return {
        details: assignment,
        course: course
    }
}