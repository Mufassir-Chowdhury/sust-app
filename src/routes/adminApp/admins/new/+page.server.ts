import { redirect } from '@sveltejs/kit';
import type { Actions } from './$types';
import { DateTime } from 'luxon';
import { db } from '$lib/Database/surreal';
import type { Department } from '$lib/models';

export async function load() {
    let department = await db.select<any>("department");
    const departmentOptions = department.map((item: Department) => {
        return {
            value: item.id,
            name: item.name,
        }
    });
    const bloodGroupOptions = [
        {name: "A+", value: "A+"},
        {name: "A-", value: "A-"},
        {name: "B+", value: "B+"},
        {name: "B-", value: "B-"},
        {name: "AB+", value: "AB+"},
        {name: "AB-", value: "AB-"},
        {name: "O+", value: "O+"},
        {name: "O-", value: "O-"},
    ]
    const genderOptions = [
        {name: "Male", value: "male"},
        {name: "Female", value: "female"},
    ]
    return {
        departmentOptions: departmentOptions,
        bloodGroupOptions: bloodGroupOptions,
        genderOptions: genderOptions
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
        // TODO handle failure
        if(record){
            throw redirect(303, "/adminApp/admins/" + "admin:" + data.get('id') + "?showSuccess=true");
        }
        
	},
} satisfies Actions;