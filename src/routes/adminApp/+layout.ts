import type { Group } from "$lib/models.js";

export function load() {

    const pageGroups: Group[] = [
        {
            "title": "Updates",
            "pages": [
                {
                    "name": "Departments",
                    "url": "/departments",
                    "icon": "fi-tr-clock-one"
                },
                {
                    "name": "Courses",
                    "url": "/courses",
                    "icon": "fi-tr-betamax"
                }, 
                {
                    "name": "Offered Courses",
                    "url": "/offeredCourses",
                    "icon": "fi-tr-border-all"
                },
            ],
        },
        {
            "title": "People",
            "pages": [
                {
                    "name": "Students",
                    "url": "/students",
                    "icon": "fi-tr-head-vr"
                },
                {
                    "name": "Teachers",
                    "url": "/teachers",
                    "icon": "fi-tr-farm"
                }, 
                {
                    "name": "Admins",
                    "url": "/admins",
                    "icon": "fi-tr-fort"
                }
            ],
        },
        {
            "title": "Administration",
            "pages": [
                {
                    "name": "Payments",
                    "url": "/payments",
                    "icon": "fi-tr-head-vr"
                },
            ],
        }
    ];
    return {
        body: pageGroups,
        appUrl: 'adminApp',
        appName: 'Admin App'
    }
    
}