import type { Group } from "$lib/models.js";

export function load() {

    const pageGroups: Group[] = [
        {
            "title": "Updates",
            "pages": [
                "routine",
                "assignments",
                "exams",
                "notices",
            ],
        },
        {
            "title": "Course Information",
            "pages": [
                "resources",
                "result",
                "attendance",
                "courses"
            ],
        },
        {
            "title": "Administration",
            "pages": [
                "payment",
                "registration",
                "links",
            ],
        },
        {
            "title": "Miscellaneous",
            "pages": [
                "bus",
                "people",
                "events"
            ],
        },
    ];
    return {
        body: pageGroups,
    }
    
}