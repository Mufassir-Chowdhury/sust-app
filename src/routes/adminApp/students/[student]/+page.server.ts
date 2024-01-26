import { db } from "$lib/Database/surreal.js";
import type { Student } from "$lib/models.js";

export async function load({ params }) {
    let [student] = await db.select<Record<string, Student>>(params.student);
    const result = await db.query<[[{name: string}]]>(
        'SELECT name from $dp',
        { "dp": student.department }
    );
    return {
        // TODO creating appropriate types
        details: student,
        department: result[0][0].name
    }
    
}