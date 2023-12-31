import { db } from "$lib/Database/surreal.js";
import type { Admin } from "$lib/models.js";

export async function load({ params }) {
    let [admin] = await db.select<Record<string, Admin>>(params.admin);
    const result = await db.query<[[{name: string}]]>(
        'SELECT name from $dp',
        { "dp": admin.department }
    );
    return {
        details: admin,
        department: result[0][0].name
    }
    
}