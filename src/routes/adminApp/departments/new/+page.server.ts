import { db } from '$lib/Database/surreal';
import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();

        const [record] = await db.create("department:" + data.get('id'), {
            building: data.get('building'),
            code: parseInt(data.get('code') as string),
            floor: parseInt(data.get('floor') as string),
            letter_code: data.get('letter-code'),
            minor_course_code: data.get('minor-course-code'),
            name: data.get('name'),
            syllabus: [],
        });

        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/departments/" + "department:" + data.get('id'));
        }

	},
} satisfies Actions;