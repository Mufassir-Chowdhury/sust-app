import type { Group } from '$lib/models';

export function load() {
    const pageGroups: Group[] = [
        {
            title: 'Teaching',
            pages: [
                {
                    name: 'Attendance',
                    url: '/attendance',
                    icon: 'fi-tr-clock-one',
                },
                // Add other teaching-related pages
            ],
        },
        // Add other page groups as needed
    ];

    return {
        body: pageGroups,
        appName: 'Teacher App',
        appUrl: 'teacherApp',
    };
}