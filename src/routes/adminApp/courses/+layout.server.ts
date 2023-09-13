import type { ListTile } from "$lib/models.js";

export function load() {

    let listItems: ListTile[] = [
        {
            name: "Intro to Computer Science",
            id: "CSE101"
        },
        {
            name: "Programming Languages",
            id: "CSE102"
        },
        {
            name: "Discrete Mathematics",
            id: "CSE203"
        }
    ]
    
    return {
        pageName: 'Courses',
        url: 'courses',
        listItems: listItems
    }
    
}