<script lang="ts">
    class NavItem {

        constructor(name: string, url: string, icon: string) {
            this.name = name;
            this.url = url;
            this.icon = icon;
        }
        name: string;
        url: string;
        icon: string;
    }
    class Group {
        constructor(title: string, pages: NavItem[]) {
            this.title = title;
            this.pages = pages;
        }
        title: string;
        pages: NavItem[];
    }
    const pages: Group[] = [
        {
            "title": "Updates",
            "pages": [
                {
                    "name": "Class Routine",
                    "url": "/studentApp/routine",
                    "icon": "fi-tr-clock-one"
                },
                {
                    "name": "Due Assignments",
                    "url": "/studentApp/assignments",
                    "icon": "fi-tr-betamax"
                }, 
                {
                    "name": "Upcoming Exams",
                    "url": "/studentApp/exams",
                    "icon": "fi-tr-border-all"
                },
                {
                    "name": "Notices",
                    "url": "/studentApp/notices",
                    "icon": "fi-tr-bring-front"
                }
            ]
        },
        {
            "title": "Course Information",
            "pages": [
                {
                    "name": "Resources",
                    "url": "/studentApp/resources",
                    "icon": "fi-tr-head-vr"
                },
                {
                    "name": "Result",
                    "url": "/studentApp/result",
                    "icon": "fi-tr-farm"
                }, 
                {
                    "name": "Attendance",
                    "url": "/studentApp/attendance",
                    "icon": "fi-tr-fort"
                }
            ]
        },
        {
            "title": "Administration",
            "pages": [
                {
                    "name": "Payment",
                    "url": "/studentApp/payment",
                    "icon": "fi-tr-house-chimney-window"
                },
                {
                    "name": "Course Registration",
                    "url": "/studentApp/registration",
                    "icon": "fi-tr-people-carry-box"
                }, 
                {
                    "name": "Important Links",
                    "url": "/studentApp/links",
                    "icon": "fi-tr-plane-prop"
                }
            ]
        },
        {
            "title": "Miscellaneous",
            "pages": [
                {
                    "name": "Bus Schedule",
                    "url": "/studentApp/bus",
                    "icon": "fi-tr-wine-glass-crack"
                },
                {
                    "name": "Student Information",
                    "url": "/studentApp/student",
                    "icon": "fi-tr-cubes"
                }, 
                {
                    "name": "Events",
                    "url": "/studentApp/events",
                    "icon": "fi-tr-diagram-sankey"
                },
                {
                    "name": "Map",
                    "url": "/studentApp/map",
                    "icon": "fi-tr-wheelchair-move"
                }
            ]
        },
    ];
    
</script>
<div class="h-full">
    {#each pages as page}
    <h1 class="text-left">
            {page.title}

        </h1>
        <div class="grid grid-cols-4 h-1/4">
            {#each page.pages as item}
            <a href={item.url} class="card variant-soft-surface text-center col-span-1 m-4 flex flex-col justify-center">
                <i class="fi {item.icon}"></i>
                {item.name}	
            </a>
        {/each}
        </div>
    {/each}
</div>