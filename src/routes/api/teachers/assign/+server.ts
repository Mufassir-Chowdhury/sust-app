import type { RequestHandler } from './$types';
import { assignTeacherToCourse } from '$lib/Database/teacher';
import { json } from '@sveltejs/kit';

export const POST: RequestHandler = async ({ request }) => {
    const requestJson = await request.json();
    await assignTeacherToCourse(requestJson.teacher, requestJson.course);

    return json({status: "ok"});
};