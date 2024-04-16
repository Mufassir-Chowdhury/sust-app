import { getAssignmentListByStudent, getCourseFromAssignment } from "$lib/Database/assignment";
import { ListTile } from "$lib/models.js";
import { getListTile } from "$lib/utils";

export async function load({ locals }) {
    let assignments = getListTile(await getAssignmentListByStudent(locals.user.id));
    // await getAssignmentListByStudent(locals.user.id).then( async (data) => {
    //     assignments = await data.map(async (assignment: any) => {
    //         const course = await getCourseFromAssignment(assignment.id);
    //         console.log(course[0])
    //         let tile = new ListTile(
    //             assignment.id,
    //             assignment.id,
    //             course[0].id,
    //             'Not Submitted',
    //             'Red',
    //             assignment.id,
    //             assignment.total_grade,
    //         );
    //         console.log(tile)
    //         return tile;
    // });
    // });
    // console.log(assignments)
    return {
        listItems: assignments
    }
    
}