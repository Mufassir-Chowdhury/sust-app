import { db } from '$lib/Database/surreal';
import { redirect, fail } from '@sveltejs/kit';
import type { Actions } from './$types';
import { DateTime } from "luxon";
import { getDepartmentOptions } from '$lib/Database/department';
import { getSelectOptions } from '$lib/utils';

export async function load() {

    const departmentOptions = getSelectOptions(await getDepartmentOptions());
    const privilageOptions = [
        {name: "Default", value: "Default"},
        {name: "CR", value: "CR"},
    ]
    return {
        departmentOptions: departmentOptions,
        privilageOptions: privilageOptions,
    }
    
}

export const actions = {
	default: async ({ request }: { request: any }) => {
        const data = await request.formData();
        const id = 'student:' + data.get('id');
       
        const [record] = await db.create("student", {
            id: id,
            blood_group: data.get('blood group'),
            department: data.get('department'),
            current_semester: parseInt(data.get('current-semester') as string),
            email: {
                academic: data.get('academic-email'),
                personal: data.get('personal-email'),
            },
            gender: data.get('gender'),
            name: data.get('name'),
            privilage: data.get('privilage'),
            personal: {
                birthday: DateTime.fromISO(data.get('date-of-birth') as string),
                father: data.get('father\'s-name'),
                mother: data.get('mother\'s-name'),
                phone: data.get('phone'),
                hometown: data.get('hometown'),
            },
            session: parseInt(data.get('session') as string),
        });
        const user = await db.query<[[any]]>('CREATE user CONTENT { email: $email, name: $name, password: crypto::argon2::generate($password) }', {email: data.get('academic-email'), name: data.get('name'), password: 'password'});
        await db.query('RELATE $student->login->$user', { student: id, user: user[0][0].id });

        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/students/" + "student:" + data.get('id') + "?showSuccess=true");
        }

	},
} satisfies Actions;