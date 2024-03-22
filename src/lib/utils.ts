import type { ListTile } from "./models";

export function getID(id: string): string{
    return id.split(':')[1];
}
export const bloodGroupOptions = [
    {name: "A+", value: "A+"},
    {name: "A-", value: "A-"},
    {name: "B+", value: "B+"},
    {name: "B-", value: "B-"},
    {name: "AB+", value: "AB+"},
    {name: "AB-", value: "AB-"},
    {name: "O+", value: "O+"},
    {name: "O-", value: "O-"},
]
export const genderOptions = [
    {name: "Male", value: "male"},
    {name: "Female", value: "female"},
]
export const typeOptions = [
    {name: "Lab", value: "Lab"},
    {name: "Theory", value: "Theory"},
]

export function getListTile(list: any): ListTile[] {
    let listItems: ListTile[] = list.map((admin: any) => {
        return {
            name: admin.name,
            id: admin.id
        }
    });
    return listItems;
}

export function getSelectOptions(list: any): any {
    return list.map((item: any) => {
        return {
            value: item.id,
            name: item.name,
        }
    });
}