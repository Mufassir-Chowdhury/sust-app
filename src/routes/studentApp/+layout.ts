import type { Group } from "$lib/models.js";

export function load() {

    const pageGroups: Group[] = [
        {
            "title": "Updates",
            "pages": [
                {
                    "name": "Class Routine",
                    "url": "/routine",
                    "icon": "fi-tr-clock-one"
                },
                {
                    "name": "Due Assignments",
                    "url": "/assignments",
                    "icon": "fi-tr-betamax"
                }, 
                {
                    "name": "Upcoming Exams",
                    "url": "/exams",
                    "icon": "fi-tr-border-all"
                },
                {
                    "name": "Notices",
                    "url": "/notices",
                    "icon": "fi-tr-bring-front"
                }
            ],
        },
        {
            "title": "Course Information",
            "pages": [
                {
                    "name": "Resources",
                    "url": "/resources",
                    "icon": "fi-tr-head-vr"
                },
                {
                    "name": "Result",
                    "url": "/result",
                    "icon": "fi-tr-farm"
                }, 
                {
                    "name": "Attendance",
                    "url": "/attendance",
                    "icon": "fi-tr-fort"
                }
            ],
        },
        {
            "title": "Administration",
            "pages": [
                {
                    "name": "Payment",
                    "url": "/payment",
                    "icon": "fi-tr-house-chimney-window"
                },
                {
                    "name": "Course Registration",
                    "url": "/registration",
                    "icon": "fi-tr-people-carry-box"
                }, 
                {
                    "name": "Important Links",
                    "url": "/links",
                    "icon": "fi-tr-plane-prop"
                }
            ],
        },
        {
            "title": "Miscellaneous",
            "pages": [
                {
                    "name": "Bus Schedule",
                    "url": "/bus",
                    "icon": "fi-tr-wine-glass-crack"
                },
                {
                    "name": "People",
                    "url": "/people",
                    "icon": "fi-tr-cubes"
                }, 
                {
                    "name": "Events",
                    "url": "/events",
                    "icon": "fi-tr-diagram-sankey"
                },
                {
                    "name": "Map",
                    "url": "/map",
                    "icon": "fi-tr-wheelchair-move"
                }
            ],
        },
    ];
    return {
        body: pageGroups,
        appName: 'Student App',
        appUrl: 'studentApp',
    }
    
}