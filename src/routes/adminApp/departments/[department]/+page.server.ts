import { db } from "$lib/Database/surreal.js";
import type { Department } from "$lib/models.js";

export async function load({ params }) {
    let [department] = await db.select<Record<string, Department>>(params.department);
    
    return {
        details: department,
    }
    
}