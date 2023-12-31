import { db } from "$lib/Database/surreal.js";
import type { Teacher } from "$lib/models.js";

export async function load({ params }) {
    let [teacher] = await db.select<Record<string, Teacher>>(params.teacher);
    const result = await db.query<[[{name: string}]]>(
        'SELECT name from $dp',
        { "dp": teacher.department }
    );
    return {
        details: teacher,
        department: result[0][0].name
    }
    
}