import { db } from '$lib/Database/surreal';
import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();
        const [record] = await db.create("notice", {
            title: data.get('title'),
            details: data.get('details'),
            creator: data.get('creator'),
            department: data.getAll('department'),
            creation_date: new Date().toLocaleDateString(),
        });
        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/notices/" + record.id + "?showSuccess=true");
        }

	},
} satisfies Actions;