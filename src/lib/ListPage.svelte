<script lang="ts">
	import type { ListTile } from "./models";

    export let pageName: string;
    export let appName: string;
    export let url: string;
    export let listItems: ListTile[];
</script>

<div class="p-8">
    <div class="flex justify-between">
        <h2 class="h2">
            {pageName}
        </h2>
        <a href="/{appName}/{url}/new">
            <button class="btn btn-primary">
                <i class="fi fi-tr-square-plus"></i>
            </button>
        </a>
    </div>
    {#each listItems as listItem}
    <a href="/{appName}/{url}/{listItem.id}">
        <div class="p-4 card card-hover variant-soft-surface my-4 flex justify-between">
            <div class="flex flex-col">
                <h3>
                    {listItem.name}
                </h3>
                <subtitle>
                    {listItem.id.split(':')[1]}
                </subtitle>
            </div>
            <div>
                <button class="btn btn-primary"><i class="fi fi-tr-file-edit"></i></button>
                <button class="btn btn-danger"><i class="fi fi-tr-delete-right"></i></button>
            </div>
        </div>
    </a>
    {/each}
</div>