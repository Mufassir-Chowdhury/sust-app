import type { RequestHandler } from './$types';
import { json } from '@sveltejs/kit';
import { registerStudentToCourses } from '$lib/Database/student';

export const POST: RequestHandler = async ({ request }) => {
    const requestJson = await request.json();
    await registerStudentToCourses(requestJson.student, requestJson.courses);

    return json({status: "ok"});
};