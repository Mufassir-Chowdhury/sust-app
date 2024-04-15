import { getDepartment } from "$lib/Database/department.js";

export async function load({ params }) {
    let department = await getDepartment(params.department);
    
    return {
        details: department,
    }
    
}