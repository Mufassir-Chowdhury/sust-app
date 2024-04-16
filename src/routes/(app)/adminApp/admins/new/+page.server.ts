import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';
import { DateTime } from 'luxon';
import { db } from '$lib/Database/surreal';
import { getDepartmentOptions } from '$lib/Database/department';
import { getSelectOptions } from '$lib/utils';

export async function load() {
    let departmentOptions = getSelectOptions(await getDepartmentOptions());
    
    return {
        departmentOptions: departmentOptions
    }
    
}

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();
        const id = 'admin:' + data.get('id');
       
        const [record] = await db.create("admin", {
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
        const user = await db.query<[[any]]>('CREATE user CONTENT { email: $email, name: $name, password: crypto::argon2::generate($password) }', {email: data.get('academic-email'), name: data.get('name'), password: 'password'});
        await db.query('RELATE $admin->login->$user', { admin: id, user: user[0][0].id });
        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/admins/" + "admin:" + data.get('id') + "?showSuccess=true");
        }
        
	},
} satisfies Actions;