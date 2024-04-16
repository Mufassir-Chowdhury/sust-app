import { getAssignment } from "$lib/Database/assignment.js";

export async function load({ params }) {
    let assignment = await getAssignment(params.assignment);
    return {
        details: assignment
    }
}