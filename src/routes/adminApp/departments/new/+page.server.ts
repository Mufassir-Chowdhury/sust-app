import { db } from '$lib/Database/surreal';
import type { Actions } from './$types';

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();
        const name = data.get('name');
        const id = data.get('id');
        const building = data.get('building');
        const letter_code = data.get('letter-code');
        const code = parseInt(data.get('code') as string);
        const floor = parseInt(data.get('floor') as string);
        const minor_course_code = data.get('minor-course-code');
        const [record] = await db.create("department:" + id, {
            building: building,
            code: code,
            floor: floor,
            letter_code: letter_code,
            minor_course_code: minor_course_code,
            name: name,
        });

        if(record){
            return {
                status: 200,
                body: {
                    message: "Department created successfully"
                }
            }
        }

	},
} satisfies Actions;