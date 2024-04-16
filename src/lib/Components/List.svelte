<script lang="ts">
	import type { ListTile } from "$lib/models";
    import { getID } from "$lib/utils";
	import { onDelete } from "$lib/Modal/DeleteModal";
	import { page } from '$app/stores';
	import { onMount } from "svelte";

    let appName: string;
    let url: string;

    $: appName = $page.url.pathname.split('/')[1];
    $: url = $page.url.pathname.split('/')[2];

    // THIS MUST BE IN table:id FORMAT
    export let listItems: ListTile[];
    export let deletable: Boolean = false;
    export let editable: Boolean = false;
    export let slug: string | undefined = undefined;

    const statusMap: Record<string, string> = {
        "green": "text-green-500",
        "red": "text-red-500",
        "yellow": "text-yellow-500",
        "black": "text-black",
    }
    let path = slug ?? "";
    onMount(() => {
        if(!slug) path = `/${appName}/${url}`;

    })
</script>

{#each listItems as listItem}
    <div class="py-4 px-8 card card-hover variant-soft-surface my-4 grid grid-cols-12">
            <a href="{path}/{listItem.id}" class="flex flex-col col-span-9">
                <div>
                    <h3>
                        {listItem.title ?? listItem.name}
                    </h3>
                    <subtitle>
                        {getID(listItem.id)}
                    </subtitle>
                </div>
            </a>
            <div class="col-span-3 flex justify-end items-center gap-8">
                {#if listItem.status}
                    <div class="  {statusMap[listItem.status_color ?? "black"]}">{listItem.status}</div>
                {/if}
                {#if listItem.trailing}
                    <div>{listItem.trailing}</div>
                {/if}
                {#if editable}
                    <button type="button" on:click={() => console.log("clicked") } class="btn btn-primary"><i class="fi fi-tr-file-edit"></i></button>
                {/if}
                {#if deletable}
                    <button type="button" on:click={() => onDelete(listItem.id) } class="btn btn-danger"><i class="fi fi-tr-delete-right"></i></button>            
                {/if}
            </div>
        </div>
{/each}
