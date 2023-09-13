import type { ListTile } from "$lib/models.js";

export function load() {

    let listItems: ListTile[] = [
        {
            name: "Computer Science and Engeering",
            id: "CSE"
        },
        {
            name: "Electronics and Communication Engineering",
            id: "ECE"
        },
        {
            name: "Electrical and Electronics Engineering",
            id: "EEE"
        }
    ]
    
    return {
        pageName: 'Departments',
        url: 'departments',
        listItems: listItems
    }
    
}