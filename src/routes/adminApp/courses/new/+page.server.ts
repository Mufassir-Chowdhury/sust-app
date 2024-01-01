import { db } from '$lib/Database/surreal';
import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';
import type { Department } from '$lib/models';

export async function load() {
    let department = await db.select<any>("department");
    const departmentOptions = department.map((item: Department) => {
        return {
            value: item.id,
            name: item.name,
        }
    });
    const typeOptions = [
        {name: "Lab", value: "Lab"},
        {name: "Theory", value: "Theory"},
    ]
    return {
        departmentOptions: departmentOptions,
        typeOptions: typeOptions,
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
            throw redirect(303, "/adminApp/courses/" + "course:" + data.get('id'));
        }

	},
} satisfies Actions;