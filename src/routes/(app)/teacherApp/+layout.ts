import type { Group } from '$lib/models';

export function load() {
    const pageGroups: Group[] = [
        {
            title: 'Teaching',
            pages: [
                "attendance",
                "assignments",
                "courses",
                "exams"
                // Add other teaching-related pages
            ],
        },
        // Add other page groups as needed
    ];

    return {
        body: pageGroups,
    };
}