import type { RequestHandler } from './$types';
import { getTeacherList } from '$lib/Database/teacher';
import { json } from '@sveltejs/kit';

export const GET: RequestHandler = async () => {
    const teachers = await getTeacherList();

	return json(teachers);
};