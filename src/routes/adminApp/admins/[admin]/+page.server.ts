import { getAdmin } from "$lib/Database/admin.js";
import { getDepartmentName } from "$lib/Database/department";

export async function load({ params }) {
    let admin = await getAdmin(params.admin);
    let departmentName = await getDepartmentName(admin.department);
    return {
        details: admin,
        department: departmentName
    }
    
}