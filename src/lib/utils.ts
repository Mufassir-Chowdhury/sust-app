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
    let listItems: ListTile[] = list.map((item: any) => {
        return {
            title: item.title,
            subtitle: item.subtitle,
            status: item.status,
            status_color: item.status_color,
            trailing: item.trailing,
            id: item.id,
            name: item.name
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