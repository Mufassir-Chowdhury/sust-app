<script lang="ts">
	import { page } from "$app/stores";
	import type { ListTile } from "../models";

    let appName: string;
    let url: string;

    $: appName = $page.url.pathname.split('/')[1];
    $: url = $page.url.pathname.split('/')[2];

    // THIS MUST BE IN table:id FORMAT
    export let listItems: ListTile[];

    const statusMap: Record<string, string> = {
        "green": "text-green-500",
        "red": "text-red-500",
        "yellow": "text-yellow-500",
        "black": "text-black",
    }
    

</script>

{#each listItems as listItem}
    <div class="py-4 px-8 card card-hover variant-soft-surface my-4 grid grid-cols-12">
        <a href="/{appName}/{url}/{listItem.id}" class="flex flex-col col-span-9">
            <div>
                <h3>
                    {listItem.title}
                </h3>
                <subtitle>
                    {listItem.subtitle}
                </subtitle>
            </div>
        </a>
        <div class="col-span-3 flex justify-end items-center gap-8">
            <div class="  {statusMap[listItem.status_color ?? "black"]}">{listItem.status}</div>
            <div>{listItem.trailing}</div>
        </div>
    </div>
{/each}
