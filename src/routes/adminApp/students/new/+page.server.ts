import { db } from '$lib/Database/surreal';
import { redirect, fail } from '@sveltejs/kit';
import type { Actions } from './$types';
import type { Department } from '$lib/models';
import { DateTime } from "luxon";

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
    const privilageOptions = [
        {name: "Default", value: "Default"},
        {name: "CR", value: "CR"},
    ]
    return {
        departmentOptions: departmentOptions,
        bloodGroupOptions: bloodGroupOptions,
        genderOptions: genderOptions,
        privilageOptions: privilageOptions,
    }
    
}

export const actions = {
	default: async ({ request }) => {
        const data = await request.formData();
        console.log(data);
       
        const [record] = await db.create("student:" + data.get('id'), {
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
        console.log(record);
        // // TODO handle failure
        // if(record){
        //     throw redirect(303, "/adminApp/students/" + "student:" + data.get('id'));
        // }

	},
} satisfies Actions;