import type { Group } from "$lib/models.js";

export function load() {

    const pageGroups: Group[] = [
        {
            "title": "Updates",
            "pages": [
                "departments",
                "courses",
                "notices"
            ],
        },
        {
            "title": "People",
            "pages": [
                "students",
                "teachers",
                "admins",
            ],  
        },
        {
            "title": "Administration",
            "pages": [
                "payment",
            ],
        }
    ];
    return {
        body: pageGroups,
    }
    
}