import { database, db } from "$lib/Database/surreal.js";

import type { Department } from "$lib/models";

export async function getDepartmentList(){
    if(!database) return [
        {
            name: "Computer Science and Engineering",
            id: "department:CSE",
        },
        {
            name: "Electrical and Electronic Engineering",
            id: "department:EEE",
        }
    ];
    return await db.select('department')
}

export async function getDepartment(id: string): Promise<any> {
    if(!database) return {
        name: "Computer Science and Engineering",
        id: "department:CSE",
        building: "Main Building",
        code: 1,
        floor: 3,
        letter_code: "CSE",
        minor_course_code: "CSE",
    } ;
    let [department] = await db.select<Record<string, Department>>(id);
    return department;
}

export async function getDepartmentName(id: string): Promise<any> {
    if(!database) return "Computer Science and Engineering";
    const result = await db.query<[[{name: string}]]>(
        'SELECT name from $dp',
        { "dp": id }
    );
    return result[0][0].name;
}

export async function getDepartmentOptions(): Promise<any> {
    if(!database) return [{ value: "cse", name: "Computer Science and Engineering" }];
    return await db.select<any>("department");
}