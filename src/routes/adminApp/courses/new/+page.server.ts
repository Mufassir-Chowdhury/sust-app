import { db } from '$lib/Database/surreal';
import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';
import { getSelectOptions } from '$lib/utils';
import { getDepartmentOptions } from '$lib/Database/department';

export async function load() {
    let departmentOptions = getSelectOptions(await getDepartmentOptions());
    
    return {
        departmentOptions: departmentOptions
    }
    
}

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();

        const [record] = await db.create("course:" + data.get('id'), {
            name: data.get('name'),
            credit: parseFloat(data.get('credit') as string),
            department: data.get('department'),
            course_code: data.get('id'),
            type: data.get('type'),
        });

        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/courses/" + "course:" + data.get('id') + "?showSuccess=true");
        }

	},
} satisfies Actions;