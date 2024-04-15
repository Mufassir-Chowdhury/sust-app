import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';
import { DateTime } from 'luxon';
import { db } from '$lib/Database/surreal';
import { getSelectOptions } from '$lib/utils';
import { getDepartmentOptions } from '$lib/Database/department';

export async function load() {
    const departmentOptions = getSelectOptions(await getDepartmentOptions());
    return {
        departmentOptions: departmentOptions
    }
    
}

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();
        const id = 'teacher:' + data.get('id');
       
        const [record] = await db.create("teacher", {
            id: id,
            blood_group: data.get('blood group'),
            department: data.get('department'),
            designation: data.get('designation'),
            email: {
                academic: data.get('academic-email'),
                personal: data.get('personal-email'),
            },
            gender: data.get('gender'),
            name: data.get('name'),
            personal: {
                birthday: DateTime.fromISO(data.get('date-of-birth') as string),
                father: data.get('father\'s-name'),
                mother: data.get('mother\'s-name'),
                phone: data.get('phone'),
                hometown: data.get('hometown'),
            },
        });
        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/teachers/" + "teacher:" + data.get('id') + "?showSuccess=true");
        }
        
	},
} satisfies Actions;