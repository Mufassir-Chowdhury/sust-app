import type { ListTile } from "$lib/models.js";

export function load() {

    let listItems: ListTile[] = [
        {
            name: "Mufassir Ahmad Chowdhury",
            id: "2019331073"
        },
        {
            name: "Sajid Zakaria",
            id: "2019331086"
        },
        {
            name: "Asanul Haque Kiron",
            id: "2019331013"
        }
    ]
    
    return {
        pageName: 'Students',
        url: 'students',
        listItems: listItems
    }
    
}