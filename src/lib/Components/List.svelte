<script lang="ts">
	import type { SimpleListTile } from "../models";
    import { getID } from "$lib/utils";
	import { onDelete } from "../Modal/DeleteModal";
	import { page } from '$app/stores';

    let appName: string;
    let url: string;

    $: appName = $page.url.pathname.split('/')[1];
    $: url = $page.url.pathname.split('/')[2];

    // THIS MUST BE IN table:id FORMAT
    export let listItems: SimpleListTile[];

    

</script>

{#each listItems as listItem}
    <div class="py-4 px-8 card card-hover variant-soft-surface my-4 grid grid-cols-12">
            <a href="/{appName}/{url}/{listItem.id}" class="flex flex-col col-span-9">
                <div>
                    <h3>
                        {listItem.name}
                    </h3>
                    <subtitle>
                        {getID(listItem.id)}
                    </subtitle>
                </div>
            </a>
            <div class="col-span-3 flex justify-end gap-8">
                <button type="button" on:click={() => console.log("clicked") } class="btn btn-primary"><i class="fi fi-tr-file-edit"></i></button>
                <button type="button" on:click={() => onDelete(listItem.id) } class="btn btn-danger"><i class="fi fi-tr-delete-right"></i></button>            
            </div>
        </div>
{/each}
