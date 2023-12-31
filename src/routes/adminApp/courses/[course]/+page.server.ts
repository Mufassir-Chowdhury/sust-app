import { db } from "$lib/Database/surreal.js";
import type { Course } from "$lib/models.js";

export async function load({ params }) {
    let [course] = await db.select<Record<string, Course>>(params.course);
    const result = await db.query<[[{name: string}]]>(
        'SELECT name from $dp',
        { "dp": course.department }
    );
    return {
        details: course,
        department: result[0][0].name
    }
    
}